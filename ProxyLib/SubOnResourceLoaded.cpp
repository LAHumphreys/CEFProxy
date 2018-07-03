/*
 * SubOnResourceLoaded.cpp
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */

#include "SubOnResourceLoaded.h"

#include "CEFDOMSnapshot.h"
#include <SimpleJSON.h>

#include <memory>

namespace
{
    static const int IDX_STATUS_CODE = 0;
    static const int IDX_URL = 1;
    static const int IDX_SOURCE = 2;

    // Hook on the renderer process to notify the browser that a load has
    // completed...
    class HandleResourceLoaded: public CefLoadHandler
    {
    public:
          void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) override
          {
              // LoadHandler triggers on both the browser and the renderer,
              // but Snapshot may only be invoked on the RENDERER!
              if (CefCurrentlyOn(TID_RENDERER)) {
                  auto msg = CefProcessMessage::Create("PAGE LOADED");
                  auto args = msg->GetArgumentList();
                  args->SetInt(IDX_STATUS_CODE, httpStatusCode);
                  args->SetString(IDX_URL, frame->GetURL());
                  args->SetString(IDX_SOURCE,
                                  std::move(CEFDOMSnapshot::GetSnapshot(frame)->GetDOMSource()));

                  browser->SendProcessMessage(PID_BROWSER, msg);
              }
          }
    private:
          IMPLEMENT_REFCOUNTING(HandleResourceLoaded);
    };

    // Subscribe to messages on the browser process and intercept the PAGE
    // LOADED messages our hook is generating...
    class ResourceLoadedMessageHandler: public CefClient
    {
    public:
        ResourceLoadedMessageHandler(SubOnResourceLoaded& sub)
            : subHandler_(sub) {}

        bool OnProcessMessageReceived(
                CefRefPtr<CefBrowser> browser,
                CefProcessId source_process,
                CefRefPtr<CefProcessMessage> message) override
        {
            if (CefCurrentlyOn(TID_UI) && message->IsValid()) {
                if (CefCurrentlyOn(TID_UI) && message->GetName() == "PAGE LOADED")
                {
                    subHandler_.NotifyClients(
                            message->GetArgumentList()->GetString(IDX_URL).ToString(),
                            message->GetArgumentList()->GetInt(IDX_STATUS_CODE),
                            message->GetArgumentList()->GetString(IDX_SOURCE).ToString());
                }
            }
            return false;
        }

        SubOnResourceLoaded& subHandler_;

        IMPLEMENT_REFCOUNTING(ResourceLoadedMessageHandler);
    };

    // Message parsers...
    NewStringField(url);
    NewIntField(status);
    NewStringField(source);

    typedef SimpleParsedJSON<url, status, source> Update;
    thread_local Update update;
}

SubOnResourceLoaded::SubOnResourceLoaded(
        CefRefPtr<CefBaseApp> app,
        IPostable& serverThread)
  : serverThread_(serverThread)
{
    app->GetClient()->LoadHandler().InstallHandler(
            std::make_shared<HandleResourceLoaded>());
    std::shared_ptr<ResourceLoadedMessageHandler> mesgHook(
            new ResourceLoadedMessageHandler(*this));
    app->GetClient()->InstallMessagerHandler(mesgHook);
}

SubOnResourceLoaded::~SubOnResourceLoaded() {
}

void SubOnResourceLoaded::OnRequest(RequestHandle hdl) {
    this->activeSessions_.push_back(hdl);
}

void SubOnResourceLoaded::NotifyClients(
        std::string url_,
        int code_,
        std::string source_)
{
    if(activeSessions_.size() > 0)
    {
        update.Clear();
        update.Get<url>() = std::move(url_);
        update.Get<status>() = code_;
        update.Get<source>() = std::move(source_);
        std::shared_ptr<std::string> msg {new std::string(update.GetJSONString())};

        for (auto it = activeSessions_.begin(); it != activeSessions_.end(); ++it) {
            auto session = *it;
            if (session->Ok() == true) {
                serverThread_.PostTask([=] () -> void {
                    if (session->Ok()) {
                        session->SendMessage(*msg);
                    }
                });
            } else {
                // TODO: prune the sessions...
            }
        }
    }
}
