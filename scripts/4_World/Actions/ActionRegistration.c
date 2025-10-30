// Inject our actions into the vanilla action registry
modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);

        actions.Insert(ActionPackVehicleVS);
        actions.Insert(ActionUnpackVehicleVS);
    }
}
