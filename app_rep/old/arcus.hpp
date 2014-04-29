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
/**
* @file arcus.hpp
* @brief 
* @author harebox <harebox@nhn.com>
* @version $Id$
* @date 2012-07-15
*/

#ifndef _ARCUS_H_
#define _ARCUS_H_

#include "log.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <libmemcached/memcached.h>
#include <time.h>

namespace arcus
{
    class hint;
    class cached_data;
    class base_client;
    class connection_pool;
    class replicated_cluster;

    class hint
    {
    public:
        typedef enum {
            NO_HINT,                /* No hint is given. But, the data is correct. */
            NO_HINT_POSSIBLY_STALE, /* No hint is given. And, the data might be incorrect. */
            HINT_MATCHED,           /* Hint is given. And, corresponding data is found */
            HINT_NOTMATCHED,        /* Hint is given. But, corresponding data is not found */
            HINT_INCORRECT          /* The cached data is different the given hint */
        } code;
    public:
        hint(uint64_t index, const char *value, size_t value_length);
        ~hint();

        //bool match(cached_data *data, bool first);
        bool match(memcached_coll_result_st *result, bool first, hint::code *state);

    private:
        uint64_t _index;
        const char *_value;
        size_t _value_length;
    };

    class cached_data
    {
    friend class base_client;
    public:
        hint::code state;

    public:
        cached_data();
        cached_data(memcached_coll_result_st *result);
        ~cached_data();

        size_t size();
        const char *value(size_t index);
        size_t value_length(size_t index);

    private:
        struct memcached_coll_result_st *_result;
    };

    /* cache client */
    class base_client
    {
    public:
        base_client();
        ~base_client();

        /*
         * Initialize a client
         * 
         * @param admin         arcus admin address (e.g.: "zk-ensemble-name:port")
         * @param service_codes service code list separated with comma
         * @param initial       initial size of connection pool
         * @param max           maximum size of connection pool
         * @param timeout       timeout vlaue (miliseconds)
         * @return sucess:true, fail:false    
         */
        bool init(std::string admin, std::string service_codes, int32_t pool_initial, int32_t pool_max, uint64_t timeout_ms);

        /*
         * Delete a given key
         *
         * @param key
         * @param rc  error code
         *     MEMCACHED_SUCCESS
         *     MEMCACHED_TIMEOUT
         * @return sucess:true, fail:false
         */
        bool del(std::string key, memcached_return_t *rc);

        /*
         * Retrieve data of from-to range on given key
         *
         * A record is implemented with b+tree structure.
         * Each field of the record is defined with bkey of uint64 type.
         *
         * @param key
         * @param from
         * @param to
         * @param hint  Hint can be given to resolve the data inconsistency.
         *              When data inconsistency occurs,
         *              the correct data should be retrieved in the following order.
         *              value in cluster 1 -> value in hint -> value in cluster 2 -> ...
         * @param result
         * @param rc
         *     MEMCACHED_SUCCESS
         *     MEMCACHED_TIMEOUT
         *     MEMCACHED_NOTFOUND
         *     MEMCACHED_NOTFOUND_ELEMENT
         * @return sucess:true, fail:false
         */
        bool get(std::string key, const uint64_t from, const uint64_t to, hint *hint,
                 cached_data *result, memcached_return_t *rc);

        /*
         * Generate a key with N fields.
         *
         * A record is created with b+tree structure.  
         *
         * @param key
         * @param num_of_items
         * @param bkeys
         * @param values
         * @param value_lengths
         * @param create_attrs
         * @param rc
         *     MEMCACHED_SUCCESS
         *     MEMCACHED_TIMEOUT
         *     MEMCACHED_EXISTS
         *     MEMCACHED_ELEMENT_EXISTS
         * @return sucess:true, fail:false
         */
        bool insert(std::string key, const size_t num_of_items,
                    const uint64_t *bkeys, const char * const *values, const size_t *value_lengths,
                    memcached_coll_create_attrs_st *create_attrs, memcached_return_t *rc);

        /*
         * Update a field on the given key.
         *
         * @param key
         * @param bkey
         * @param value
         * @param value_length
         * @param rc
         *     MEMCACHED_SUCCESS
         *     MEMCACHED_TIMEOUT
         *     MEMCACHED_NOTFOUND
         *     MEMCACHED_NOTFOUND_ELEMENT
         * @return sucess:true, fail:false
         */
        bool update(std::string key, const uint64_t bkey, const char *value, const size_t value_length,
                    memcached_return_t *rc);

        /* 
         * Return a coonection pool of given position.
         */
        connection_pool *pool(size_t i);

    protected:
        replicated_cluster *_cluster;
    };

    /* cache cluster connection pool */
    class connection_pool
    {
    public:
        connection_pool(std::string admin, std::string service_code, int32_t initial, int32_t max, uint64_t timeout);
        ~connection_pool();

        void release(memcached_st *handle, memcached_return_t *rc);
        memcached_st *fetch(memcached_return_t *rc, struct timespec *timeout);

        const char *to_string();

    private:
        void destroy();

    private:
        std::string _admin;
        std::string _service_code;
        int32_t _initial;
        int32_t _max;
        int32_t _timeout;

        memcached_st *_master_handle;
        memcached_pool_st *_pool;

        std::string _str;
    };

    /* multiple connection pool management */
    class replicated_cluster
    {
    public:
        virtual bool add(std::string admin, std::string service_code, int32_t initial, int32_t max, uint64_t timeout);

        size_t pool_size();

        connection_pool *pool(size_t i);

    protected:
        std::vector<connection_pool *> _pools;
    };

} // namespace arcus

#endif
