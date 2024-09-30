#include "Module.h"

namespace WPEFramework
{
    namespace Plugin
    {
        // This is a server for a JSONRPC communication channel.
        // For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
        // By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
        // This realization of this interface implements, by default, the following methods on this plugin
        // - exists
        // - register
        // - unregister
        // Any other methood to be handled by this plugin  can be added can be added by using the
        // templated methods Register on the PluginHost::JSONRPC class.
        // As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
        // this class exposes a public method called, Notify(), using this methods, all subscribed clients
        // will receive a JSONRPC message as a notification, in case this method is called.
        class SmartControl : public PluginHost::IPlugin, public PluginHost::JSONRPC
        {
        public:
            // constants
            static const short API_VERSION_NUMBER_MAJOR;
            static const short API_VERSION_NUMBER_MINOR;
	    static const string SERVICE_NAME;

            // methods
            static const string METHOD_SMARTCONTROL_SET_VOLUME_LEVEL;
            static const string METHOD_SMARTCONTROL_GET_VOLUME_LEVEL;


            SmartControl();
            virtual ~SmartControl();
            virtual const string Initialize(PluginHost::IShell *shell) override;
            virtual void Deinitialize(PluginHost::IShell *service) override;
	    virtual string Information() const override;


            BEGIN_INTERFACE_MAP(SmartControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            END_INTERFACE_MAP

            PluginHost::IShell *m_CurrentService;
	    static SmartControl *_instance;

        private:
            bool m_isInitialized;
	    WPEFramework::JSONRPC::LinkType<WPEFramework::Core::JSON::IElement> *m_DisplaySettingsPluginObj = NULL;

            bool ds_getVolumeLevel(JsonObject &Result);
            bool ds_setVolumeLevel(const JsonObject &parameters, JsonObject &Result);
            bool ThresholdLevelCheck();

            void onVolumeLevelChangedHandler(const JsonObject &parameters);
            void onDeviceOnHandler(const JsonObject &parameters);


            void getThunderPlugins();

            // We do not allow this plugin to be copied !!
            SmartControl(const SmartControl &) = delete;
            SmartControl &operator=(const SmartControl &) = delete;
        };
    } // namespace Plugin
} // namespace WPEFramework
