class CfgPatches
{
    class VehicleStorageMod
    {
        units[] = { "VS_VehicleReceipt" };
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            // We will likely touch Expansion vehicles/keys later,
            // but we'll leave Expansion out of requiredAddons for now to avoid hard dependency breakage.
            // We'll add DayZ Expansion classes to requiredAddons once we actually reference them.
        };
    };
};

class CfgMods
{
    class VehicleStorageMod
    {
        dir = "VehicleStorageMod";
        name = "Vehicle Storage Mod";
        picture = "";
        credits = "You";
        author = "You";
        authorID = "0";
        version = "1.0";
        type = "mod";

        // Tell the game where our Enforce Script files live
        dependencies[] = { "Game", "World", "Mission" };

        class defs
        {
            class gameScriptModule
            {
                files[] = { "VehicleStorageMod/scripts/3_Game" };
            };
            class worldScriptModule
            {
                files[] = { "VehicleStorageMod/scripts/4_World" };
            };
            class missionScriptModule
            {
                files[] = { "VehicleStorageMod/scripts/5_Mission" };
            };
        };
    };
};

// Register our custom item (the 'receipt')
// We inherit from Paper so it's spawnable, stackable-ish, lightweight.
class CfgVehicles
{
    class Paper; // vanilla base class for paper notes

    class VS_VehicleReceipt: Paper
    {
        scope = 2;
        displayName = "Vehicle Storage Receipt";
        descriptionShort = "Stores a packed vehicle. Use near vehicle to pack/unpack.";
        // Reuse the paper model so it looks like a piece of paper for now
        model = "\dz\gear\consumables\Paper.p3d";

        // We'll eventually inject tooltip data (vehicle name, etc.).
        // For now it's just a normal paper with a unique classname.
    };
};
