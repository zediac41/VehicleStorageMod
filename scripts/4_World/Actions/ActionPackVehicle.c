// VehicleStorageMod/scripts/4_World/actions/actionpackvehicle.c

class ActionPackVehicleVSCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		// 5 second progress bar
		m_ActionData.m_ActionComponent = new CAContinuousTime(5.0);
	}
}

class ActionPackVehicleVS: ActionContinuousBase
{
	void ActionPackVehicleVS()
	{
		m_CallbackClass = ActionPackVehicleVSCB; // version-safe (no anim constants)
	}

	override void CreateConditionComponents()
	{
		// We need to point at a world object (the vehicle or any of its parts)
		m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
		// Receipt in hands must not be ruined
		m_ConditionItem   = new CCINonRuined();
	}

	override string GetText()
	{
		return "Pack Vehicle";
	}

	override bool HasTarget() { return true; }

	// Resolve the actual vehicle even if cursor hits a door/fender/wheel proxy.
	protected EntityAI GetRootVehicleFromTarget(ActionTarget target)
	{
		if (!target) return null;
		Object obj = target.GetObject();
		if (!obj) return null;

		// Walk up the hierarchy using GetParent()
		Object cur = obj;
		for (int i = 0; i < 8 && cur; i++)
		{
			// Reached a Transport (vehicle)?
			if (Transport.Cast(cur))
				return EntityAI.Cast(cur);

			cur = cur.GetParent(); // <- Enforce DayZ API
		}

		// Fallback: if the first object already is a Transport
		Transport tr = Transport.Cast(obj);
		if (tr) return EntityAI.Cast(obj);

		return null;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!player || !item || !target) return false;

		// Must be holding our receipt
		VS_VehicleReceipt receipt = VS_VehicleReceipt.Cast(item);
		if (!receipt) return false;

		// Receipt must look EMPTY on the client
		if (receipt.VS_ClientThinksFull())
			return false;

		// Always resolve root vehicle from any hit selection (door, fender, etc.)
		EntityAI veh = GetRootVehicleFromTarget(target);
		if (!veh) return false;
		if (!Transport.Cast(veh)) return false;

		// 1) Vehicle must be unlocked (Expansion lock check)
		if (!VS_KeyAccess.IsVehicleUnlocked(veh))
			return false;

		// 2) Player must have the paired key for THIS vehicle (or admin key)
		string vName = VS_KeyAccess.VehicleDisplayNameFromObject(veh);
		if (!VS_KeyAccess.FindValidKeyForName(player, vName))
			return false;

		// Optional distance sanity
		if (vector.Distance(player.GetPosition(), veh.GetPosition()) > 8.0)
			return false;

		return true;
	}

	// SERVER: perform the pack after the bar completes
	override void OnFinishProgressServer(ActionData action_data)
	{
		if (!action_data) return;

		PlayerBase player = PlayerBase.Cast(action_data.m_Player);
		if (!player) return;

		VS_VehicleReceipt receipt = VS_VehicleReceipt.Cast(action_data.m_MainItem);
		if (!receipt) return;

		// Resolve root vehicle here too (server-authoritative)
		EntityAI veh = null;
		{
			Object obj = action_data.m_Target.GetObject();
			if (!obj) return;

			Object cur = obj;
			for (int i = 0; i < 8 && cur; i++)
			{
				if (Transport.Cast(cur))
				{
					veh = EntityAI.Cast(cur);
					break;
				}
				cur = cur.GetParent(); // <- Enforce DayZ API
			}
			if (!veh)
			{
				Transport tr = Transport.Cast(obj);
				if (tr) veh = EntityAI.Cast(obj);
			}
		}
		if (!veh) return;

		VS_VehicleStorageManager.Get().PackVehicleIntoReceipt(veh, receipt, player);
	}
}
