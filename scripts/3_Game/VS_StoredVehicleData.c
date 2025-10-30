class VS_AttachmentData
{
    string m_Type;        // classname of the attachment (wheel, battery, etc.)
    string m_SlotName;    // placeholder for future slot restore
}

class VS_StoredVehicleData
{
    string m_VehicleType;                          // classname of the vehicle
    ref array<ref VS_AttachmentData> m_Attachments; // attachments only (no cargo)

    void VS_StoredVehicleData()
    {
        m_Attachments = new array<ref VS_AttachmentData>();
    }

    int GetAttachmentCount()
    {
        if (!m_Attachments)
            return 0;
        return m_Attachments.Count();
    }

    string ToReadableSummary()
    {
        return string.Format("%1 (%2 parts)", m_VehicleType, GetAttachmentCount().ToString());
    }

    // --- Serialization for persistence ---
    //
    // We'll store data in a compact string like:
    // VehicleClass|Wheel_FrontLeft,Whell_FrontRight,Battery,...
    //
    // m_SlotName will come later when we actually restore per-slot.

    string Serialize()
    {
        string result = m_VehicleType + "|";

        for (int i = 0; i < m_Attachments.Count(); i++)
        {
            VS_AttachmentData ad = m_Attachments[i];
            if (!ad)
                continue;

            result += ad.m_Type;

            if (i < m_Attachments.Count() - 1)
            {
                result += ",";
            }
        }

        return result;
    }

    // Turn the serialized string back into structured data.
    static VS_StoredVehicleData Deserialize(string serialized)
    {
        VS_StoredVehicleData data = new VS_StoredVehicleData();

        array<string> parts = new array<string>;
        serialized.Split("|", parts);

        if (parts.Count() > 0)
        {
            data.m_VehicleType = parts[0];
        }

        if (parts.Count() > 1)
        {
            string attachmentsBlob = parts[1];

            array<string> attList = new array<string>;
            attachmentsBlob.Split(",", attList);

            for (int i = 0; i < attList.Count(); i++)
            {
                string attType = attList[i];
                if (attType == "")
                    continue;

                VS_AttachmentData ad = new VS_AttachmentData();
                ad.m_Type = attType;
                ad.m_SlotName = ""; // slot restore logic comes later
                data.m_Attachments.Insert(ad);
            }
        }

        return data;
    }
}
