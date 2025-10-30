// VehicleStorageMod/scripts/4_World/actions/actionunpackvehicle.c

class ActionUnpackVehicleVSCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(5.0); // 5s bar
	}
}

class ActionUnpackVehicleVS: ActionContinuousBase
{
	void ActionUnpackVehicleVS()
	{
		m_CallbackClass = ActionUnpackVehicleVSCB;
	}

	override void CreateConditionComponents()
	{
		m_ConditionTarget = new CCTNone();       // item-in-hands action
		m_ConditionItem   = new CCINonRuined();  // receipt can't be ruined
	}

	override string GetText()
	{
		return "Unpack Vehicle";
	}

	override bool HasTarget()
	{
		return false; // no world target required
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!player || !item) return false;

		VS_VehicleReceipt receipt = VS_VehicleReceipt.Cast(item);
		if (!receipt) return false;

		// Client-side heuristic: shows when the receipt *appears* full
		// (server will still do final key/space checks when the bar completes)
		if (!receipt.VS_ClientThinksFull())
			return false;

		return true;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		if (!action_data) return;

		PlayerBase player = PlayerBase.Cast(action_data.m_Player);
		if (!player) return;

		VS_VehicleReceipt receipt = VS_VehicleReceipt.Cast(action_data.m_MainItem);
		if (!receipt) return;

		// Server enforces: correct key/admin key + space + everything else
		VS_VehicleStorageManager.Get().UnpackVehicleFromReceipt(player, receipt);
	}
}
