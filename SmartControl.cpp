#include <algorithm>
#include <regex>
#if defined(SECURITY_TOKEN_ENABLED) && ((SECURITY_TOKEN_ENABLED == 0) || (SECURITY_TOKEN_ENABLED == false))
#define GetSecurityToken(a, b) 0
#define GetToken(a, b, c) 0
#else
#include <WPEFramework/securityagent/securityagent.h>
#include <WPEFramework/securityagent/SecurityTokenUtil.h>
#endif
#include "SmartControl.h"
#include "UtilsJsonRpc.h"
#include <fstream>


const string WPEFramework::Plugin::SmartControl::SERVICE_NAME = "org.rdk.SmarControl";
const string WPEFramework::Plugin::SmartControl::METHOD_SMARTCONTROL_SET_VOLUME_LEVEL = "ds_setVolumeLevel";
const string WPEFramework::Plugin::SmartControl::METHOD_SMARTCONTROL_GET_VOLUME_LEVEL = "ds_getvolumeLevel";

using namespace std;

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

#define SERVER_DETAILS "127.0.0.1:9998"
#define DISPLAYSETTINGS_CALLSIGN_VER ".1"
#define DISPLAYSETTINGS_CALLSIGN "org.rdk.DisplaySettings"
#define BLUETOOTH_CALLSIGN_VER ".1"
#define BLUETOOTH_CALLSIGN  "org.rdk.Bluetooth"
#define SECURITY_TOKEN_LEN_MAX 1024
#define THUNDER_RPC_TIMEOUT 2000
#define VOLUME_THRESHOLD_LEVEL 4

namespace WPEFramework
{
	namespace
	{
		static Plugin::Metadata<Plugin::SmartControl> metadata(
			// Version (Major, Minor, Patch)
			API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
			// Preconditions
			{},
			// Terminations
			{},
			// Controls
			{});
	}

	namespace Plugin
	{
		SERVICE_REGISTRATION(SmartControl, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

		SmartControl *SmartControl::_instance = nullptr;

                SmartControl::SmartControl()
                        : PluginHost::JSONRPC(),
                          m_isInitialized(false),


		{
			SmartControl::_instance = this;

			Register(METHOD_SMARTCONTROL_SET_VOLUME_LEVEL, &SmartControl::ds_setVolumeLevel, this);
			Register(METHOD_SMARTCONTROL_GET_VOLUME_LEVEL, &SmartControl::ds_getVolumeLevel, this);

		}

		SmartControl::~SmartControl()
		{
			if (nullptr != m_DisplaySettingsPluginObj)
			{
				delete m_DisplaySettingsPluginObj;
				m_DisplaySettingsPluginObj = nullptr;
			}

                        if (nullptr != m_BluetoothPluginObj)
                        {
                                delete m_BluetoothPluginObj;
                                m_BluetoothPluginObj = nullptr;
                        }

			Unregister(METHOD_SMARTCONTROL_SET_VOLUME_LEVEL);
			Unregister(METHOD_SMARTCONTROL_GET_VOLUME_LEVEL);
		}


		// Thunder plugins communication
		void SmartControl::getThunderPlugins()
		{
			if (nullptr == m_DisplaySettingsPluginObj)
			{
				string token;
				// TODO: use interfaces and remove token
				auto security = m_CurrentService->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
				if (nullptr != security)
				{
					string payload = "http://localhost";
					if (security->CreateToken(static_cast<uint16_t>(payload.length()),
											  reinterpret_cast<const uint8_t *>(payload.c_str()),
											  token) == Core::ERROR_NONE)
					{
						LOGINFO("got security token\n");
					}
					else
					{
						LOGERR("failed to get security token\n");
					}
					security->Release();
				}
				else
				{
					LOGERR("No security agent\n");
				}

				string query = "token=" + token;
				Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T(SERVER_DETAILS)));
				m_DisplaySettingsPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(DISPLAYSETTINGS_CALLSIGN_VER), (_T("SmartControl")), false, query);
				if (nullptr == m_DisplaySettingsPluginObj)
				{
					LOGERR("JSONRPC: %s: initialization failed", DISPLAYSETTINGS_CALLSIGN_VER);
				}
				else
				{
					LOGINFO("JSONRPC: %s: initialization ok", DISPLAYSETTINGS_CALLSIGN_VER);
				}

				m_BluetoothPluginObj = new WPEFramework::JSONRPC::LinkType<Core::JSON::IElement>(_T(BLUETOOTH_CALLSIGN_VER), (_T("SmartControl")), false, query);
                                if (nullptr == m_BluetoothPluginObj)
                                {
                                        LOGERR("JSONRPC: %s: initialization failed", BLUETOOTH_CALLSIGN_VER);
                                }
                                else
                                {
                                        LOGINFO("JSONRPC: %s: initialization ok", BLUETOOTH_CALLSIGN_VER);
                                }
			}
		}

		const string SmartControl::Initialize(PluginHost::IShell *service)
		{
                    if (!m_isInitialized)
		    {

			getThunderPlugins();

			m_CurrentService = service;

			//event subscribe
			
			if (nullptr != m_DisplaySettingsPluginObj)
                        {
                            m_DisplaySettingsPluginObj-->Subscribe<JsonObject>(1000, "volumeLevelChanged", &SmartControl::onVolumeLevelChangedHandler, this);
                        }
                        
			if (nullptr != m_BluetoothPluginObj)
                        {
                            m_BluetoothPluginObj-->Subscribe<JsonObject>(1000, "onDeviceFound", &SmartControl::onDeviceFoundHandler, this);
                        }

			m_isInitialized = true;
		    }

                    return (string());
		}

		void SmartControl::Deinitialize(PluginHost::IShell * /* service */)
		{
			SmartControl::_instance = nullptr;

			if(m_isInitialized)
			{
                            m_isInitialized = false;
			    m_CurrentService = nullptr;
			}

		}

                string SmartControl::Information() const
                {
                        return (string("{\"service\": \"") + SERVICE_NAME + string("\"}"));
                }

                bool SmartControl::ds_getVolumeLevel(JsonObject &Result)
		{
			bool result = false;
			JsonObject params;
			uint32_t Vlevel;
			LOGINFO("Entering..!!!");

			getThunderPlugins();

			if (nullptr == m_DisplaySettingsPluginObj)
			{
				LOGERR("m_DisplaySettingsPluginObj not yet instantiated");
				return result;
			}

			uint32_t ret = m_DisplaySettingsPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("getVolumeLevel"), params, Result);
			if (Core::ERROR_NONE == ret)
			{
				if (Result["success"].Boolean())
				{
					Vlevel = Result["volumeLevel"].Number();
					LOGINFO("ds_getVolumeLevel value=%u", Vlevel);
					result = true;
				}
				else
				{
					ret = Core::ERROR_GENERAL;
					LOGERR("ds_getVolumeLevel call failed");
				}
			}
			else
			{
				LOGERR("ds_getVolumeLevel failed E[%u]", ret);
			}

			return result;
		}


                bool SmartControl::ds_setVolumeLevel(const JsonObject &parameters, JsonObject &Result)
                {
                    bool result = false;
                    if ((parameters.HasLabel("volumeLevel")) && (parameters.HasLabel("audioPort")))
		    {
                        getThunderPlugins();

                        if (nullptr == m_DisplaySettingsPluginObj)
                        {
                            LOGERR("m_DisplaySettingsPluginObj not yet instantiated");
                            return false;
                        }

                        uint32_t ret = m_DisplaySettingsPluginObj->Invoke<JsonObject, JsonObject>(THUNDER_RPC_TIMEOUT, _T("setVolumeLevel"), parameters, Result);
                        if (Core::ERROR_NONE == ret)
                        {
                                if (Result["success"].Boolean())
                                {
                                    result = true;
                                }
                                else
                                {
                                    ret = Core::ERROR_GENERAL;
                                    LOGERR("ds_setVolumeLevel call failed");
                                }
                        }
                        else
                        {
                            LOGERR("ds_setVolumeLevel failed E[%u]", ret);
                        }
                   }
                   return result;
               }

                bool ThresholdLevelCheck()
                {
                    bool result = false;
                    JsonObject Internal;
                    JsonObject InternalResponse;

                    if (ds_getVolumeLevel(Internal) )
                    {
                        if (Internal["volumelevel"].Number() <= VOLUME_THRESHOLD_LEVEL )
                        {
                            Internal["volumelevel"] = VOLUME_THRESHOLD_LEVEL;
                            Internal["audioPort"] = "SPEAKER0";

                            //Set the Volumelevel in threshold level
                            if (ds_setVolumeLevel(Internal,InternalResponse))
                                result = true;
                        }
			else
			{
                            LOGERR("Unable to set to Threshold level")
			}
                    }

                    return result;
                }

		void onVolumeLevelChangedHandler(const JsonObject &parameters)
		{
                        string message;
			JsonObject InternalResponse;
                        parameters.ToString(message);

			LOGINFO("[VolumeLevel Event], [%s]", message.c_str());

                        if ( parameters["volumelevel"].Number() == VOLUME_THRESHOLD_LEVEL )
			{
                            //update the database 
                            LOGINFO("Current Volume Level : %d", InternalResponse["volumelevel"].Number());  
			}
		}


                void onDeviceOnHandler(const JsonObject &parameters)
                {
                        string message;
			parameters.ToString(message);
                        LOGINFO("[OnDevice Event], [%s]", message.c_str());
                        
			ThresholdLevelCheck();
                }

	} // namespace Plugin
} // namespace WPEFramework
