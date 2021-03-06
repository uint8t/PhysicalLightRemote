

void FillLightData(Light_Collection *lightCollection, int targetLightIndex, 
                   const char *ipBuffer, Features *featuresBuffer, 
                   bool isPowered, int brightness)
{
    using namespace Yeelight;    
    Light *light = &lightCollection->lights[targetLightIndex];
    
    CopyString(light->ipAddress, ipBuffer);
    light->features = *featuresBuffer;
    light->isPowered = isPowered;
    light->brightness = brightness;

    if (!light->features.setRgb)
    {
        lightCollection->hasAnyWhiteLight = true;
    }

#if DEV_PRINT
    Print("Adding new light with IP: ");
    Print(light->ipAddress);
    Print(", features: [");
    if (light->features.setPower)
    {
        Print(SetPower);
        Print(", ");   
    }
    if (light->features.setBright)
    {
        Print(SetBright);
        Print(", ");   
    }
    if (light->features.setRgb)
    {
        Print(SetRgb);
        Print(", ");   
    }
    if (light->features.setCtAbx)
    {
        Print(SetCtAbx);
        Print(", ");   
    }
    if (light->features.startCf)
    {
        Print(StartCf);
        Print(", ");   
    }
    if (light->features.stopCf)
    {
        Print(StopCf);
        Print(", ");   
    }
    Print("], Power state: ");
    Print(light->isPowered);
    Print(", Brightness: ");
    PrintN(light->brightness);

#endif
}

void ParseUdpRead(Light_Collection *lightCollection, const char *buffer)
{
    const char yeelightTag[] = "yeelight://";
    const char colonTag[] = ":";
    const char supportTag[] = "support:";
    const char powerTag[] = "power:";
    const char brightTag[] = "bright:";
    const char colorModeTag[] = "color_mode";

    int addressOffset = FindFirstOf(buffer, yeelightTag);
    if (addressOffset != -1)
    {
        buffer += addressOffset + sizeof(yeelightTag) - 1;

        int colonOffset = FindFirstOf(buffer, colonTag);
        if (colonOffset != -1 && colonOffset < 16)
        {
            char ipBuffer[16];
            CatString(ipBuffer, buffer, 0, colonOffset);

            for (int lightIndex = 0; 
                 lightIndex < lightCollection->currentLightCount;
                 ++lightIndex)
            {
                if (AreStringIdentical(ipBuffer, lightCollection->lights[lightIndex].ipAddress))
                {
                    return;
                }
            }

            int supportOffset = FindFirstOf(buffer, supportTag);
            if (supportOffset != -1)
            {
                buffer += supportOffset + sizeof(supportTag) - 1;
                int powerOffset = FindFirstOf(buffer, powerTag);

                if (powerOffset != -1)
                {
                    using namespace Yeelight;

                    Features featuresBuffer = {0};
                    featuresBuffer.setPower = (FindFirstOf(buffer, SetPower, powerOffset) != -1);
                    featuresBuffer.setBright = (FindFirstOf(buffer, SetBright, powerOffset) != -1);
                    featuresBuffer.setRgb = (FindFirstOf(buffer, SetRgb, powerOffset) != -1);
                    featuresBuffer.setCtAbx = (FindFirstOf(buffer, SetCtAbx, powerOffset) != -1);
                    featuresBuffer.startCf = (FindFirstOf(buffer, StartCf, powerOffset) != -1);
                    featuresBuffer.stopCf = (FindFirstOf(buffer, StopCf, powerOffset) != -1);

                    buffer += powerOffset + sizeof(powerTag) + 1;
                    bool isPowered = (*buffer == 'n');
                    
                    int brightness = 100;
                    int brightOffset = FindFirstOf(buffer, brightTag);
                    int colorModeOffset = FindFirstOf(buffer, colorModeTag);
                    if ((brightOffset != -1) && (colorModeOffset != -1))
                    {
                        brightOffset += sizeof(brightTag);

                        char brightBuffer[SmallBufferSize] = {0};
                        CatString(brightBuffer, buffer, brightOffset, (colorModeOffset - brightOffset));
                        brightness = Clamp<int>(atoi(brightBuffer), 1, 100);
                    }
                    

                    FillLightData(lightCollection, lightCollection->currentLightCount, ipBuffer, &featuresBuffer, isPowered, brightness);
                    ++lightCollection->currentLightCount;
                }

            }

        }
    }
}


void UdpReadMultipleMessages(WiFiUDP *udp, Light_Collection *lightCollection)
{
    for (int networkReadIndex = 0; 
     networkReadIndex < 10;
     ++networkReadIndex)
    {
        char buffer[BigBufferSize] = {0};
        if (UdpRead(udp, buffer, sizeof(buffer)))
        {
            ParseUdpRead(lightCollection, buffer);
        }
        else
        {
            // Print("End of Messages, received: ");
            // PrintN(networkReadIndex + 1);
            break;
        }
    }
}

