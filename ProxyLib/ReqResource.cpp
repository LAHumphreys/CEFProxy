/*
 * ReqResource.cpp
 *
 *  Created on: 25 Feb 2018
 *      Author: lhumphreys
 */


#include "ReqResource.h"
#include <SimpleJSON.h>

#include <include/cef_urlrequest.h>

namespace
{
    NewStringField(url);
    NewStringField(postData);
    NewIntField(status);
    NewStringField(document);
    namespace header_fields {
        NewStringField(name);
        NewStringField(value);
        using header = SimpleParsedJSON<name, value>;
    }
    using HeaderObject = header_fields::header;
    NewObjectArray(headers, header_fields::header);
    typedef SimpleParsedJSON<url, postData, headers> Request;
    typedef SimpleParsedJSON<status, document> Reply;
    Request request;

    class URLRequestClient: public CefURLRequestClient
    {
    public:
        URLRequestClient(ReqGET& hdlr, const size_t& id)
           : handler_(hdlr)
           , id_(id) {}

    private:
        ReqGET& handler_;
        size_t id_;
        std::string data_;

        // URLRequestClient INTERFACE IMPLEMENTATION
        virtual void OnRequestComplete(CefRefPtr<CefURLRequest> request) {
            // We'll be called back in the UI process, (specifcally TID_UI)
            // ... but we need to be on the server thread to dispatch a
            // reply
            const int code = request->GetResponse()->GetStatus();

            handler_.RequestFullfilled(id_, code, std::move(data_));
        }

        // Use the information to prevent lots of copies of the string...
        void OnDownloadProgress(CefRefPtr<CefURLRequest> request,
                                int64 current,
                                int64 total) override {
            if (total > 0) {
                data_.reserve(total);
            } else {
                // CEF has been unable to determine the size of the download...
            }
        }

        // Some more data's been retrieved - grab it!
        void OnDownloadData(CefRefPtr<CefURLRequest> request,
                            const void* data,
                            size_t data_length) override {
            const char* start = reinterpret_cast<const char*>(data);
            data_.append(start, data_length);
        }

        // Don't care for our simple one-shot request reply use case
        // (Should never trigger upload...)
        void OnUploadProgress(CefRefPtr<CefURLRequest> request,
                              int64 current,
                              int64 total) override { }



        // See the note in the parent class. For our simple use case
        // just permit the damned thing...
        bool GetAuthCredentials(bool isProxy,
                                const CefString& host,
                                int port,
                                const CefString& realm,
                                const CefString& scheme,
                                CefRefPtr<CefAuthCallback> callback) {
            return true;
        }

        IMPLEMENT_REFCOUNTING(URLRequestClient);

    };
}

ReqGET::ReqGET(
        IPostable& server,
        ProxyBrowserHandler& browser)
   : server_(server)
   , browser_(browser)
{
}

void ReqGET::OnRequest(RequestHandle hdl) {
    static std::string error;
    request.Clear();


    if ( !request.Parse(hdl->RequestMessasge(),error) ) {
        throw InvalidRequestException{0,error};
    }

    auto id = store_.StoreRequest(hdl);
    std::map<std::string, std::string> raw_headers;
    for (auto& hp: request.Get<headers>()) {
        HeaderObject& h = *hp;
        raw_headers.insert({
            h.Get<header_fields::name>(), h.Get<header_fields::value>()
        });
    }
    std::string raw_url = request.Get<url>();
    std::string raw_post_data = request.Get<postData>();

    // Must be on a valid CEF thread to fire the request
    CefBaseThread::PostToCEFThread(TID_UI,
    [this, id, raw_url, raw_post_data, raw_headers] () -> void{
        auto getRequest = CefRequest::Create();
        getRequest->SetURL(raw_url);

        CefRequest::HeaderMap headerMap;
        for (auto& hp: raw_headers) {
            headerMap.insert({hp.first, hp.second});
        }

        if (!raw_post_data.empty()) {
            auto data = CefPostData::Create();
            auto element = CefPostDataElement::Create();
            auto strData = raw_post_data;
            element->SetToBytes(strData.length(), strData.c_str());
            data->AddElement(element);
            getRequest->SetMethod("POST");
            getRequest->SetPostData(data);
            headerMap.insert({ "Content-Type", "application/json"});
            headerMap.insert(
                {"Accept", "application/json, text/javascript, */*; q=0.01" });
            getRequest->SetHeaderMap(headerMap);
            // Grant it access to our cookies...
            getRequest->SetFlags(UR_FLAG_ALLOW_STORED_CREDENTIALS);
        } else {
            getRequest->SetFlags(UR_FLAG_ALLOW_STORED_CREDENTIALS);
            getRequest->SetMethod("GET");
            getRequest->SetHeaderMap(headerMap);
        }

        CefRefPtr<URLRequestClient> client = new URLRequestClient(*this, id);
        CefURLRequest::Create(getRequest,client, nullptr);
    });

}

size_t ReqGET::RequestStore::StoreRequest(RequestHandle hdl) {
    size_t thisIdx = ++idx;
    store_[thisIdx] = hdl;

    return thisIdx;
}

SubscriptionHandler::RequestHandle
ReqGET::RequestStore::Release(const size_t& idx) {
    RequestHandle result(nullptr);

    auto it = store_.find(idx);
    if (it != store_.end())
    {
        result = std::move(it->second);
        store_.erase(it);
    }

    return result;

}

void ReqGET::RequestFullfilled(const size_t& id, int code, std::string data) {
    server_.PostTask([=] () -> void {
        Reply reply;
        reply.Clear();
        reply.Get<status>() = code;
        reply.Get<document>() = std::move(data);

        auto hdl = this->store_.Release(id);

        hdl->SendMessage(reply.GetJSONString());
        hdl->Close();
    });
}
