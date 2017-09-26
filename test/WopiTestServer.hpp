/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "config.h"

#include "Log.hpp"
#include "Unit.hpp"
#include "UnitHTTP.hpp"
#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/JSON/Object.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/URI.h>

class WopiTestServer : public UnitWSD
{
public:
    WopiTestServer() : UnitWSD()
    {
    }

    virtual void assertCheckFileInfoRequest(const Poco::Net::HTTPRequest& request) = 0;

    virtual void assertGetFileRequest(const Poco::Net::HTTPRequest& request) = 0;

    virtual bool wopiServerFinish() = 0;

protected:
    /// Here we act as a WOPI server, so that we have a server that responds to
    /// the wopi requests without additional expensive setup.
    virtual bool handleHttpRequest(const Poco::Net::HTTPRequest& request, std::shared_ptr<StreamSocket>& socket) override
    {
        static const std::string hello("Hello, world");

        Poco::URI uriReq(request.getURI());
        LOG_INF("Fake wopi host request: " << uriReq.toString());

        // CheckFileInfo
        if (uriReq.getPath() == "/wopi/files/0" || uriReq.getPath() == "/wopi/files/1")
        {
            LOG_INF("Fake wopi host request, handling CheckFileInfo: " << uriReq.getPath());

            assertCheckFileInfoRequest(request);

            Poco::LocalDateTime now;
            Poco::JSON::Object::Ptr fileInfo = new Poco::JSON::Object();
            fileInfo->set("BaseFileName", "hello.txt");
            fileInfo->set("Size", hello.size());
            fileInfo->set("Version", "1.0");
            fileInfo->set("OwnerId", "test");
            fileInfo->set("UserId", "test");
            fileInfo->set("UserFriendlyName", "test");
            fileInfo->set("UserCanWrite", "true");
            fileInfo->set("PostMessageOrigin", "localhost");
            fileInfo->set("LastModifiedTime", Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::ISO8601_FORMAT));

            std::ostringstream jsonStream;
            fileInfo->stringify(jsonStream);
            std::string responseString = jsonStream.str();

            const std::string mimeType = "application/json; charset=utf-8";

            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Last-Modified: " << Poco::DateTimeFormatter::format(Poco::Timestamp(), Poco::DateTimeFormat::HTTP_FORMAT) << "\r\n"
                << "User-Agent: " << HTTP_AGENT_STRING << "\r\n"
                << "Content-Length: " << responseString.size() << "\r\n"
                << "Content-Type: " << mimeType << "\r\n"
                << "\r\n"
                << responseString;

            socket->send(oss.str());
            socket->shutdown();

            return true;
        }
        // GetFile
        else if (uriReq.getPath() == "/wopi/files/0/contents" || uriReq.getPath() == "/wopi/files/1/contents")
        {
            LOG_INF("Fake wopi host request, handling GetFile: " << uriReq.getPath());

            assertGetFileRequest(request);

            const std::string mimeType = "text/plain; charset=utf-8";

            std::ostringstream oss;
            oss << "HTTP/1.1 200 OK\r\n"
                << "Last-Modified: " << Poco::DateTimeFormatter::format(Poco::Timestamp(), Poco::DateTimeFormat::HTTP_FORMAT) << "\r\n"
                << "User-Agent: " << HTTP_AGENT_STRING << "\r\n"
                << "Content-Length: " << hello.size() << "\r\n"
                << "Content-Type: " << mimeType << "\r\n"
                << "\r\n"
                << hello;

            socket->send(oss.str());
            socket->shutdown();

            if (wopiServerFinish())
                exitTest(TestResult::Ok);

            return true;
        }

        return false;
    }

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
