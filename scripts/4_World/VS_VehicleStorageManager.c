// VehicleStorageMod/scripts/4_World/VS_VehicleStorageManager.c

class VS_VehicleStorageManager
{
	protected static ref VS_VehicleStorageManager m_Instance;
	static VS_VehicleStorageManager Get()
	{
		if (!m_Instance) m_Instance = new VS_VehicleStorageManager();
		return m_Instance;
	}

	void VS_VehicleStorageManager() {}

	// Where we stash vehicles while “packed” (far outside playable map)
	static const vector VS_STORAGE_POS = "100000 0 100000";

	// -------------------------------- Helpers --------------------------------
	protected void SendPlayerMessage(PlayerBase player, string msg)
	{
		if (player) player.MessageStatus("[VehicleStorage] " + msg);
	}

	protected bool IsPackableVehicle(EntityAI vehicle)
	{
		return vehicle && Transport.Cast(vehicle);
	}

	protected bool ReceiptAvailable(VS_VehicleReceipt receipt)
	{
		return receipt && !receipt.VS_HasStoredVehicle();
	}

	protected bool PlayerHasValidKeyForVehicle(PlayerBase player, EntityAI vehicle)
	{
		return VS_KeyAccess.FindValidKey(player, vehicle) != null;
	}

	protected bool VehicleHasPassengers(EntityAI vehicle)
	{
		Transport t = Transport.Cast(vehicle);
		if (!t) return false;

		int n = t.CrewSize();
		for (int i = 0; i < n; i++)
		{
			if (Human.Cast(t.CrewMember(i))) return true;
		}
		return false;
	}

	protected bool VehicleHasLooseCargo(EntityAI vehicle)
	{
		if (!vehicle) return false;
		CargoBase cargo = vehicle.GetInventory().GetCargo();
		return cargo && cargo.GetItemCount() > 0;
	}

	protected VS_StoredVehicleData BuildSnapshot(EntityAI vehicle)
	{
		VS_StoredVehicleData data = new VS_StoredVehicleData();
		data.m_VehicleType = vehicle.GetType();

		if (vehicle.GetInventory())
		{
			int attCount = vehicle.GetInventory().AttachmentCount();
			for (int i = 0; i < attCount; i++)
			{
				EntityAI att = vehicle.GetInventory().GetAttachmentFromIndex(i);
				if (!att) continue;

				VS_AttachmentData ad = new VS_AttachmentData();
				ad.m_Type = att.GetType();
				ad.m_SlotName = ""; // reserved for slot-accurate restore later
				data.m_Attachments.Insert(ad);
			}
		}
		return data;
	}

	// ------------------------------- PACK ------------------------------------
	bool CanPackVehicle(EntityAI vehicle, PlayerBase player, VS_VehicleReceipt receipt)
	{
		if (!player || !vehicle || !receipt) return false;
		if (!IsPackableVehicle(vehicle)) return false;
		if (!ReceiptAvailable(receipt)) return false;

		// Must be unlocked (Expansion lock check)
		if (!VS_KeyAccess.IsVehicleUnlocked(vehicle)) return false;

		// Must have paired key or admin key
		if (!PlayerHasValidKeyForVehicle(player, vehicle)) return false;

		// No passengers or loose cargo
		if (VehicleHasPassengers(vehicle)) return false;
		if (VehicleHasLooseCargo(vehicle)) return false;

		return true;
	}

	bool PackVehicleIntoReceipt(EntityAI vehicle, VS_VehicleReceipt receipt, PlayerBase player)
	{
		if (!CanPackVehicle(vehicle, player, receipt))
		{
			SendPlayerMessage(player, "Cannot pack: needs correct key, empty cargo, no passengers, unlocked, and empty receipt.");
			return false;
		}

		// Build snapshot for UI (class + attachment summary)
		VS_StoredVehicleData data = BuildSnapshot(vehicle);
		receipt.VS_SetStoredData(data);

		// --- Stash the exact vehicle instance instead of deleting it ---
		receipt.VS_CaptureHiddenVehicle(vehicle); // stores persistent ID + original pose on the receipt

		// Move to stash location & keep orientation benign
		vehicle.SetPosition(VS_STORAGE_POS);
		vehicle.SetOrientation("0 0 0");
		vehicle.SetAllowDamage(false);

		int attachCount = data.m_Attachments.Count();
		SendPlayerMessage(player, "Packed " + data.m_VehicleType + " with " + attachCount.ToString() + " attachment(s).");

		// Update client label immediately
		receipt.VS_OnJustPacked(player);
		return true;
	}

	// ------------------------------ UNPACK -----------------------------------
	protected bool IsSpawnAreaClear(vector pos, float radius)
	{
		array<Object> objects = new array<Object>;
		array<CargoBase> cargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition3D(pos, radius, objects, cargos);

		for (int i = 0; i < objects.Count(); i++)
		{
			if (Transport.Cast(objects[i])) return false;
		}
		return true;
	}

	protected bool FindValidSpawnLocation(PlayerBase player, out vector outPos, out vector outOri)
	{
		if (!player) return false;

		vector p = player.GetPosition();
		vector f = player.GetDirection();
		vector o = player.GetOrientation();
		float dists[3] = {3.0, 4.0, 5.0};

		for (int i = 0; i < 3; i++)
		{
			vector testPos = p + (f * dists[i]);
			testPos[1] = GetGame().SurfaceY(testPos[0], testPos[2]);

			if (IsSpawnAreaClear(testPos, 3.0))
			{
				outPos = testPos; outOri = o; return true;
			}
		}
		return false;
	}

	// Find the hidden vehicle at the stash spot by its persistent ID
	protected EntityAI FindHiddenVehicleById(int id1, int id2, int id3, int id4)
	{
		array<Object> objects = new array<Object>;
		array<CargoBase> cargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition3D(VS_STORAGE_POS, 50.0, objects, cargos);

		for (int i = 0; i < objects.Count(); i++)
		{
			EntityAI ea = EntityAI.Cast(objects[i]);
			if (!ea) continue;
			if (!Transport.Cast(ea)) continue;

			int a, b, c, d;
			ea.GetPersistentID(a, b, c, d);
			if (a == id1 && b == id2 && c == id3 && d == id4)
				return ea;
		}
		return null;
	}

	bool CanUnpackVehicle(PlayerBase player, VS_VehicleReceipt receipt)
	{
		if (!player || !receipt) return false;
		if (!receipt.VS_HasStoredVehicle()) return false;

		vector dummyPos, dummyOri;
		return FindValidSpawnLocation(player, dummyPos, dummyOri);
	}

	bool UnpackVehicleFromReceipt(PlayerBase player, VS_VehicleReceipt receipt)
	{
		if (!CanUnpackVehicle(player, receipt))
		{
			SendPlayerMessage(player, "Cannot unpack here. Not enough space or receipt is empty.");
			return false;
		}

		VS_StoredVehicleData data = receipt.VS_GetStoredData();
		if (!data)
		{
			SendPlayerMessage(player, "No stored vehicle data.");
			return false;
		}

		// Strict key on UNPACK as well (paired key or admin key)
		string storedDisplay = VS_KeyAccess.VehicleDisplayNameFromType(data.m_VehicleType);
		if (!VS_KeyAccess.FindValidKeyForName(player, storedDisplay))
		{
			SendPlayerMessage(player, "You need the correct key (or admin key) to unpack this vehicle.");
			return false;
		}

		vector spawnPos, spawnOri;
		if (!FindValidSpawnLocation(player, spawnPos, spawnOri))
		{
			SendPlayerMessage(player, "No valid spawn position found.");
			return false;
		}

		EntityAI veh = null;

		// Bring back the exact original vehicle if possible (use getter for protected IDs)
		if (receipt.VS_HasHiddenVehicleStored())
		{
			int id1, id2, id3, id4;
			receipt.VS_GetHiddenVehiclePID(id1, id2, id3, id4);

			veh = FindHiddenVehicleById(id1, id2, id3, id4);
			if (veh)
			{
				veh.SetAllowDamage(true);
				veh.SetPosition(spawnPos);
				veh.SetOrientation(spawnOri);
			}
		}

		// Fallback: create a fresh one if we somehow didn't find it
		if (!veh)
		{
			int flags = ECE_PLACE_ON_SURFACE | ECE_SETUP;
			Object spawnedObj = GetGame().CreateObjectEx(data.m_VehicleType, spawnPos, flags);
			veh = EntityAI.Cast(spawnedObj);
			Transport tr = Transport.Cast(veh);
			if (!veh || !tr)
			{
				if (spawnedObj) GetGame().ObjectDelete(spawnedObj);
				SendPlayerMessage(player, "Failed to spawn vehicle.");
				return false;
			}

			veh.SetOrientation(spawnOri);

			// Reattach saved attachments (summary only)
			if (data.m_Attachments && data.m_Attachments.Count() > 0)
			{
				for (int i = 0; i < data.m_Attachments.Count(); i++)
				{
					VS_AttachmentData ad = data.m_Attachments[i];
					if (!ad) continue;
					veh.GetInventory().CreateAttachment(ad.m_Type);
				}
			}
		}

		// Clear the receipt and update client label
		receipt.VS_ClearStoredData();
		receipt.VS_OnJustUnpacked(player);

		SendPlayerMessage(player, "Unpacked " + data.m_VehicleType + ".");
		return true;
	}
}
