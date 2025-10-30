// VehicleStorageMod/scripts/4_World/items/vs_vehiclereceipt.c

class VS_VehicleReceipt: Paper
{
	// Server-side structured data used for UI/summary (classname + attachments).
	protected ref VS_StoredVehicleData m_VSData;

	// Serialized snapshot we save to hive (for UI after restart).
	protected string m_VSSerializedData;

	// Client-side label ("OffroadHatchback, 5 parts") synced via RPC so the UI shows what’s inside.
	protected string m_VSClientLabel;

	// --- Hidden vehicle tracking (to preserve key pairing) ---
	protected bool   m_VSStoredHidden;     // true if we stashed the real vehicle instead of deleting it
	protected vector m_VSHiddenPrevPos;    // where it was when packed (for info/debug)
	protected vector m_VSHiddenPrevOri;    // original orientation when packed
	protected int    m_VSId1;              // persistent ID of the stashed vehicle
	protected int    m_VSId2;
	protected int    m_VSId3;
	protected int    m_VSId4;

	static const int VS_RPC_SYNC_LABEL = 777777;

	void VS_VehicleReceipt() {}

	// ---------------- RPC label sync ----------------
	protected void VS_SendLabelToPlayer(PlayerBase player)
	{
		if (!GetGame().IsServer() || !player)
			return;

		string label = "";

		if (m_VSData)
		{
			int pc1 = m_VSData.GetAttachmentCount();
			label = m_VSData.m_VehicleType + ", " + pc1.ToString() + " parts";
		}
		else if (m_VSSerializedData != "")
		{
			VS_StoredVehicleData tmp = VS_StoredVehicleData.Deserialize(m_VSSerializedData);
			if (tmp)
			{
				int pc2 = tmp.GetAttachmentCount();
				label = tmp.m_VehicleType + ", " + pc2.ToString() + " parts";
			}
		}

		Param1<string> p = new Param1<string>(label);
		GetGame().RPCSingleParam(this, VS_RPC_SYNC_LABEL, p, true, player.GetIdentity());
	}

	void VS_OnJustPacked(PlayerBase player)
	{
		if (GetGame().IsServer() && player)
			VS_SendLabelToPlayer(player);
	}

	void VS_OnJustUnpacked(PlayerBase player)
	{
		if (GetGame().IsServer() && player)
			VS_SendLabelToPlayer(player);
	}

	override void OnInventoryEnter(Man player)
	{
		super.OnInventoryEnter(player);

		if (GetGame().IsServer())
		{
			PlayerBase pb = PlayerBase.Cast(player);
			if (pb)
				VS_SendLabelToPlayer(pb);
		}
	}

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (rpc_type == VS_RPC_SYNC_LABEL)
		{
			Param1<string> p;
			if (!ctx.Read(p))
				return;

			m_VSClientLabel = p.param1;
		}
	}

	// ---------------- Public helpers ----------------
	bool VS_HasStoredVehicle()
	{
		return m_VSData != null;
	}

	VS_StoredVehicleData VS_GetStoredData()
	{
		return m_VSData;
	}

	// Client heuristic: treat as "full" if we either have server data or a synced label
	bool VS_ClientThinksFull()
	{
		if (m_VSData)
			return true;
		if (m_VSClientLabel != "")
			return true;
		return false;
	}

	void VS_ClearStoredData()
	{
		m_VSData = null;
		m_VSSerializedData = "";
		m_VSClientLabel = "";
		m_VSStoredHidden = false;
		m_VSHiddenPrevPos = "0 0 0";
		m_VSHiddenPrevOri = "0 0 0";
		m_VSId1 = 0;
		m_VSId2 = 0;
		m_VSId3 = 0;
		m_VSId4 = 0;
		SetSynchDirty();
	}

	void VS_SetStoredData(VS_StoredVehicleData data)
	{
		m_VSData = data;

		if (m_VSData)
			m_VSSerializedData = m_VSData.Serialize();
		else
			m_VSSerializedData = "";

		SetSynchDirty();
	}

	// Capture persistent ID + original pose for hidden storage
	void VS_CaptureHiddenVehicle(EntityAI vehicle)
	{
		if (!vehicle)
			return;

		m_VSStoredHidden = true;
		m_VSHiddenPrevPos = vehicle.GetPosition();
		m_VSHiddenPrevOri = vehicle.GetOrientation();
		vehicle.GetPersistentID(m_VSId1, m_VSId2, m_VSId3, m_VSId4);
	}

	bool VS_HasHiddenVehicleStored()
	{
		return m_VSStoredHidden && (m_VSId1 != 0 || m_VSId2 != 0 || m_VSId3 != 0 || m_VSId4 != 0);
	}

	// Expose hidden vehicle PID to other classes without touching protected fields
	void VS_GetHiddenVehiclePID(out int id1, out int id2, out int id3, out int id4)
	{
		id1 = m_VSId1;
		id2 = m_VSId2;
		id3 = m_VSId3;
		id4 = m_VSId4;
	}

	// ---------------- UI / display ----------------
	override string GetDisplayName()
	{
		string baseName = super.GetDisplayName();

		if (m_VSData)
		{
			int partCount = m_VSData.GetAttachmentCount();
			return baseName + " (" + m_VSData.m_VehicleType + ", " + partCount.ToString() + " parts)";
		}

		if (m_VSClientLabel != "")
			return baseName + " (" + m_VSClientLabel + ")";

		return baseName;
	}

	// ---------------- Persistence ----------------
	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);

		// Serialized snapshot (UI summary)
		ctx.Write(m_VSSerializedData);

		// Hidden vehicle state
		ctx.Write(m_VSStoredHidden);
		ctx.Write(m_VSHiddenPrevPos);
		ctx.Write(m_VSHiddenPrevOri);
		ctx.Write(m_VSId1);
		ctx.Write(m_VSId2);
		ctx.Write(m_VSId3);
		ctx.Write(m_VSId4);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;

		// Snapshot
		ctx.Read(m_VSSerializedData);
		if (m_VSSerializedData != "")
			m_VSData = VS_StoredVehicleData.Deserialize(m_VSSerializedData);
		else
			m_VSData = null;

		// Hidden vehicle state
		ctx.Read(m_VSStoredHidden);
		ctx.Read(m_VSHiddenPrevPos);
		ctx.Read(m_VSHiddenPrevOri);
		ctx.Read(m_VSId1);
		ctx.Read(m_VSId2);
		ctx.Read(m_VSId3);
		ctx.Read(m_VSId4);

		m_VSClientLabel = "";
		SetSynchDirty();
		return true;
	}

	// ---------------- Actions ----------------
	override void SetActions()
	{
		super.SetActions();

		AddAction(ActionPackVehicleVS);
		AddAction(ActionUnpackVehicleVS);
	}
}
