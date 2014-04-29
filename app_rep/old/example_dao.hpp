/*
 * Copyright 2012-2014 NAVER Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _EXAMPLE_DOMAIN_H_
#define _EXAMPLE_DOMAIN_H_

#include "arcus.hpp"
#include <iostream>
#include <time.h>

namespace dao
{
    /**
     * Cacheable interface
     */
    class cacheable
    {
    public:
        cacheable(arcus::base_client *client)
        : _client(client)
        {
        }

        bool del()
        {
            memcached_return_t rc;
            return _client->del(_key, &rc);
        }

        virtual bool get() = 0;
        virtual bool get(size_t index, const char *hint, size_t hint_length) = 0;
        virtual bool insert() = 0;

    protected:
        arcus::base_client *_client;
        std::string _key;
    };


    /**
     * DOMAIN : USERINFO 
     */
    class userinfo : public cacheable
    {
    public:
        const static int32_t USERINFO_EXPIRETIME = 600;
        const static int32_t USERINFO_FLAGS = 0;
        const static int32_t USERINFO_NUM_OF_FIELDS = 7;

    public:
        std::string userid;
        std::string state;
        std::string contact;
        std::string srcaddr;
        struct timeval regitime;
        struct timeval lasttime;
        std::string cancel_pending; 

    public:
        userinfo(arcus::base_client *client, std::string prefix, std::string key)
        : cacheable(client)
        {
            _key = prefix + ":userinfo_" + key;
        }

        bool insert()
        {
            const uint64_t bkeys[] = { 0, 1, 2, 3, 4, 5, 6 };
            const char *values[] = {
                userid.c_str(),
                state.c_str(),
                contact.c_str(),
                srcaddr.c_str(),
                (const char *)&regitime,
                (const char *)&lasttime,
                cancel_pending.c_str()
            };
            size_t value_lengths[] = {
                userid.length(),
                state.length(),
                contact.length(),
                srcaddr.length(),
                sizeof(struct timeval),
                sizeof(struct timeval),
                cancel_pending.length()
            };

            memcached_coll_create_attrs_st create_attrs;
            memcached_coll_create_attrs_init(&create_attrs, USERINFO_FLAGS, USERINFO_EXPIRETIME, 100);

            memcached_return_t rc;
            bool result = _client->insert(_key, USERINFO_NUM_OF_FIELDS, bkeys, values, value_lengths, &create_attrs, &rc);

            return result;
        }

        bool get()
        {
            return get(0, NULL, 0);
        }

        bool get(size_t index, const char *hint, size_t hint_length)
        {
            memcached_return_t rc;
            arcus::cached_data data;

            bool success = _client->get(_key, 0, 10, NULL, &data, &rc);

            if (success) {
                if (data.size() == USERINFO_NUM_OF_FIELDS) {
                    userid         = std::string(data.value(0));
                    state          = std::string(data.value(1));
                    contact        = std::string(data.value(2));
                    srcaddr        = std::string(data.value(3));
                    regitime       = *(struct timeval *)data.value(4);
                    lasttime       = *(struct timeval *)data.value(5);
                    cancel_pending = std::string(data.value(6));
                } else {
                    _client->del(_key, &rc);
                    success = false;
                }
            }

            return success;
        }

        bool updateState(std::string state)
        {
            memcached_return_t rc;
            return _client->update(_key, 2, state.c_str(), state.length(), &rc);
        }
    };

    /**
     * DOMAIN : MESGINFO
     */
    class mesginfo : public cacheable
    {
    public:
        const static int32_t MESGINFO_EXPIRETIME = 600;
        const static int32_t MESGINFO_FLAGS = 0;
        const static int32_t MESGINFO_NUM_OF_FIELDS = 9;

    public:
        std::string sid;
        std::string state;
        std::string type;
        std::string from;
        std::string to;
        struct timeval starttime;
        struct timeval endtime;
        std::string srcip;
        std::string dstip;

    public:
        mesginfo(arcus::base_client *client, std::string prefix, std::string key)
        : cacheable(client)
        {
            _key = prefix + ":mesginfo_" + key;
        }

        virtual bool insert()
        {
            const uint64_t bkeys[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
            const char *values[] = {
                sid.c_str(),
                state.c_str(),
                type.c_str(),
                from.c_str(),
                to.c_str(),
                (const char *)&starttime,
                (const char *)&endtime,
                srcip.c_str(),
                dstip.c_str()
            };
            size_t value_lengths[] = {
                sid.length(),
                state.length(),
                type.length(),
                from.length(),
                to.length(),
                sizeof(struct timeval),
                sizeof(struct timeval),
                sizeof(struct timeval),
                srcip.length(),
                dstip.length()
            };

            memcached_coll_create_attrs_st create_attrs;
            memcached_coll_create_attrs_init(&create_attrs, MESGINFO_FLAGS, MESGINFO_EXPIRETIME, 100);

            memcached_return_t rc;
            bool result = _client->insert(_key, MESGINFO_NUM_OF_FIELDS, bkeys, values, value_lengths, &create_attrs, &rc);

            return result;
        }

        virtual bool get()
        {
            return get(0, NULL, 0);
        }

        virtual bool get(size_t index, const char *hint, size_t hint_length)
        {
            memcached_return_t rc;
            arcus::cached_data data;

            bool success = _client->get(_key, 0, 10, NULL, &data, &rc);

            if (success) {
                if (data.size() == MESGINFO_NUM_OF_FIELDS) {
                    sid       = std::string(data.value(0));
                    state     = std::string(data.value(1));
                    type      = std::string(data.value(2));
                    from      = std::string(data.value(3));
                    to        = std::string(data.value(4));
                    starttime = *(struct timeval *)data.value(5);
                    endtime   = *(struct timeval *)data.value(6);
                    srcip     = std::string(data.value(7));
                    dstip     = std::string(data.value(8));
                } else {
                    _client->del(_key, &rc);
                    success = false;
                }
            }

            return success;
        }
    };


} // namespace dao

#endif
