// VehicleStorageMod/scripts/4_World/vs/vs_keyaccess.c

class ExpansionVehicleBase;

class VS_KeyAccess
{
	static bool IsAdminKey(ItemBase item)
	{
		return item && item.IsKindOf("ExpansionCarAdminKey");
	}

	static bool IsExpansionKey(ItemBase item)
	{
		return item && item.IsKindOf("ExpansionCarKey");
	}

	static string VehicleDisplayNameFromType(string typeName)
	{
		string dn;
		if (GetGame().ConfigGetText("CfgVehicles " + typeName + " displayName", dn))
			return dn;
		return typeName;
	}

	static string VehicleDisplayNameFromObject(Object vehicle)
	{
		if (!vehicle) return "";
		return VehicleDisplayNameFromType(vehicle.GetType());
	}

	static bool KeyMatchesVehicleName(ItemBase key, string vehicleDisplayName)
	{
		if (!key || !IsExpansionKey(key)) return false;

		string k = key.GetDisplayName();
		if (k == "") k = key.GetType();

		k.ToLower();
		string vn = vehicleDisplayName;
		vn.ToLower();

		return k.Contains(vn);
	}

	static ItemBase FindValidKeyForName(PlayerBase player, string vehicleDisplayName)
	{
		if (!player) return null;

		array<EntityAI> items = new array<EntityAI>;
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		for (int i = 0; i < items.Count(); i++)
		{
			ItemBase it = ItemBase.Cast(items[i]);
			if (!it) continue;

			if (IsAdminKey(it))
				return it;

			if (KeyMatchesVehicleName(it, vehicleDisplayName))
				return it;
		}
		return null;
	}

	static ItemBase FindValidKey(PlayerBase player, Object vehicle)
	{
		return FindValidKeyForName(player, VehicleDisplayNameFromObject(vehicle));
	}

	static bool IsVehicleLocked(Object obj)
	{
		ExpansionVehicleBase ex = ExpansionVehicleBase.Cast(obj);
		if (ex) return ex.IsLocked();
		return false;
	}

	static bool IsVehicleUnlocked(Object obj)
	{
		return !IsVehicleLocked(obj);
	}
}
