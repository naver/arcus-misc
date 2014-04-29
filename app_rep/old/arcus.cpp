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
* @file arcus.cpp
* @brief 
* @author harebox <harebox@nhn.com>, junhyun park <junhyun.park@nhn.com>
* @version $Id$
* @date 2012-07-15
*/
#include "arcus.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <libmemcached/memcached.h>
#include <time.h>

#define ENABLE_TWO_COPY_REP 1 // by junhyun

namespace arcus
{
    struct timespec DEFAULT_POOL_FETCH_TIMEOUT = { 1, 0 }; 
    const size_t    DEFAULT_OPERATION_RETRY_COUNT = 2;
    const int32_t   DEFAULT_EXPIRE_TIME = 60 * 60 * 6;

    /* CONNECTION POOL */

    connection_pool::connection_pool(std::string admin, std::string service_code, int32_t initial, int32_t max, uint64_t timeout)
    : _admin(admin), _service_code(service_code), _initial(initial), _max(max), _timeout(timeout)
    {
        _master_handle = memcached_create(NULL);
        memcached_behavior_set(_master_handle, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, timeout);
        _pool = memcached_pool_create(_master_handle, initial, max); 

        if (not _pool) {
            std::cout << "memcached_pool_create() failed" << std::endl;
            return;
        }

        arcus_return_t rc = arcus_pool_connect(_pool, admin.c_str(), service_code.c_str());

        if (rc != ARCUS_SUCCESS) {
            std::cout << "arcus_pool_connect() failed" << std::endl;
            destroy();
        }

        std::stringstream stream;
        stream << "arcus::connection_pool[admin=" << _admin << ", service_code=" << _service_code << ", initial=" << _initial << ", max=" << _max << ", timeout=" << _timeout << "ms]";
        _str = stream.str();
    }

    connection_pool::~connection_pool()
    {
        destroy();
    }

    void connection_pool::release(memcached_st *handle, memcached_return_t *rc)
    {
        memcached_return_t unused;
        if (not rc) {
            rc = &unused;
        }

        *rc = memcached_pool_release(_pool, handle);
    }

    memcached_st *connection_pool::fetch(memcached_return_t *rc, struct timespec *timeout = &DEFAULT_POOL_FETCH_TIMEOUT)
    {
        return memcached_pool_fetch(_pool, timeout, rc);
    }

    const char *connection_pool::to_string()
    {
        return _str.c_str();
    }

    void connection_pool::destroy()
    {
        if (_pool) {
            memcached_pool_destroy(_pool);
            _pool = NULL;
        }

        if (_master_handle) {
            memcached_free(_master_handle);
            _master_handle = NULL;
        }
    }

    /* REPLICATED_CLUSTER */

    bool replicated_cluster::add(std::string admin, std::string service_code, int32_t initial, int32_t max, uint64_t timeout)
    {
        connection_pool *pool = new connection_pool(admin, service_code, initial, max, timeout);
        _pools.push_back(pool);
        DEBUG("created a connection pool for the cluster (%s)", pool->to_string());
        return true;
    }

    size_t replicated_cluster::pool_size()
    {
        return _pools.size();
    }

    connection_pool *replicated_cluster::pool(size_t i)
    {
        return _pools[i];
    }

    /* HINT */

    hint::hint(uint64_t index, const char *value, size_t value_length)
        : _index(index), _value(value), _value_length(value_length) { }

    hint::~hint() { }

    bool hint::match(memcached_coll_result_st *result, bool first, hint::code *state)
    {
        if (result == NULL) {
            return false;
        }

        if (_value == NULL) {
            *state = (first) ? NO_HINT : NO_HINT_POSSIBLY_STALE;
            return true;
        }

        if (strncmp(_value, memcached_coll_result_get_value(result, _index), _value_length) == 0) {
            *state = HINT_MATCHED;
        } else {
            *state = (first) ? HINT_INCORRECT : HINT_NOTMATCHED;
        }

        return true;
    }

    /* CACHED_DATA */

    cached_data::cached_data()
    {
    }

    cached_data::cached_data(memcached_coll_result_st *result)
        : _result(result)
    {
    }

    cached_data::~cached_data()
    {
        if (_result) {
            memcached_coll_result_free(_result);
            _result = NULL;
        }
    }

    size_t cached_data::size()
    {
        return (_result) ? memcached_coll_result_get_count(_result) : 0;
    }

    const char *cached_data::value(size_t index)
    {
        return (_result and index < size()) ? memcached_coll_result_get_value(_result, index) : NULL;
    }

    size_t cached_data::value_length(size_t index)
    {
        return (_result and index < size()) ? memcached_coll_result_get_value_length(_result, index) : NULL;
    }

    /* BASE_CLIENT */

    base_client::base_client()
    {
        _cluster = new replicated_cluster();
    }

    base_client::~base_client()
    {
        delete _cluster;
    }

    bool base_client::init(std::string admin, std::string service_codes, int32_t initial, int32_t max, uint64_t timeout)
    {
        std::stringstream stream(service_codes);
        std::string code;

        while (std::getline(stream, code, ',')) {
            _cluster->add(admin, code, initial, max, timeout);
        }

        return true;
    }

#ifdef ENABLE_TWO_COPY_REP
    bool base_client::remove(std::string key, memcached_return_t *rc)
    {
        memcached_st *handle;

        if (_bkup_pool != NULL) {
            handle = _bkup_pool->fetch(rc);
            if (handle == NULL) { /* what should we do ? */ }
            *rc = memcached_delete(handle, key.c_str(), key.length(), 0);
            _bkup_pool->release(handle, NULL);
            if (*rc != MEMCACHED_SUCCESS and *rc != MEMCACHED_NOTFOUND and *rc != MEMCACHED_NO_SERVERS)
                return false;
        }

        if (1) { /* _main_pool != NULL */
            handle = _main_pool->fetch(rc);
            if (handle == NULL) { /* what should we do ? */ }
            *rc = memcached_delete(handle, key.c_str(), key.length(), 0);
            _main_pool->release(handle, NULL);
        }
       
        /* Error Cases: 
         *     MEMCACHED_INVALID_ARGUMENTS
         *     MEMCACHED_NO_SERVERS 
         *     MEMCACHED_NOTFOUND
         *     ....
         */
        return (*rc == MEMCACHED_SUCCESS);
    }
#else
    bool base_client::del(std::string key, memcached_return_t *rc)
    {
        int run_count = 0;

        for (int i = 0; i < _cluster->pool_size(); ++i) {
            connection_pool *pool = _cluster->pool(i); 

            memcached_st *handle = pool->fetch(rc);

            for (int r = 0; handle, r < DEFAULT_OPERATION_RETRY_COUNT; ++r) {
                *rc = memcached_delete(handle, key.c_str(), key.length(), 0);

                if (*rc == MEMCACHED_SUCCESS or *rc == MEMCACHED_NOTFOUND) ++run_count;
                if (*rc != MEMCACHED_TIMEOUT) break;
            }

            pool->release(handle, NULL);
        }

        return (run_count >= 1);
    }  
#endif

    bool base_client::get(std::string key, const uint64_t from, const uint64_t to,
                          hint *hint, cached_data *result, memcached_return_t *rc)
    {
#ifdef ENABLE_TWO_COPY_REP
        memcached_coll_result_st coll_vals;
        
        if (1) { /* _main_pool != NULL */
           
            handle = _main_pool->fetch(rc);
            if (handle == NULL) { /* what should we do ? */ }

            memcached_coll_result_create(handle, &coll_vals);
            *rc = memcached_bop_get_by_range(handle, key.c_str(), key.length(), from, to,
                                             NULL, 0, 0, false, false, &coll_vals);

            _main_pool->release(handle, NULL);
        } 

        if (_bkup_pool != NULL)  {
        }
#else
        std::vector<memcached_coll_result_st> v(_cluster->pool_size());
        std::vector<memcached_return_t> r(_cluster->pool_size());
        *result = NULL;
        int result_index = -1;

        int run_count = 0;

        // retrieve data from all of the cache clusters
        for (int i = 0; i < _cluster->pool_size(); ++i) {
            connection_pool *pool = _cluster->pool(i); 

            memcached_st *handle = pool->fetch(rc);
            memcached_coll_result_create(handle, &v[i]);

            // get data by range. retry if timeout occurs.
            for (int j = 0; handle, j < DEFAULT_OPERATION_RETRY_COUNT; ++j) {
                r[i] = memcached_bop_get_by_range(handle, key.c_str(), key.length(), from, to, NULL, 0, 0, false, false, &v[i]);

                if (r[i] == MEMCACHED_SUCCESS) ++run_count;
                if (r[i] != MEMCACHED_TIMEOUT) break;
            }

            pool->release(handle, NULL);

            if (r[i] != MEMCACHED_SUCCESS) {
                ERROR("get failed : %s (%s)", memcached_strerror(NULL, r[i]), pool->to_string());
                continue;
            }

            bool first = (i == 0);

            // if it's the value of master, use it as return value. 
            if (result == NULL or result->_result == NULL) {
                DEBUG("got a master value (%s)", pool->to_string());
                if (result == NULL) {
                    result = new cached_data(&v[i]);
                }

                result->_result = &v[i];
                result_index = i;

                if (hint) {
                    hint->match(result->_result, first, &result->state);
                } else {
                    result->state = first ? hint::NO_HINT : hint::NO_HINT_POSSIBLY_STALE;
                }
            } else if (result->state == hint::HINT_NOTMATCHED) {
                DEBUG("trying to find next value matching with the hint");
                hint::code state;
                hint->match(&v[i], first, &state);

                if (state == hint::HINT_MATCHED) {
                    result->_result = &v[i];
                    result->state = state;
                    result_index = i;
                }
            } else {
                DEBUG("got a slave value  (%s)", pool->to_string());
            }

            // decide the clusters that must be synchronized.
            if (i == 0) {
                continue;
            } else if (r[i-1] != MEMCACHED_SUCCESS and r[i] == MEMCACHED_SUCCESS) {
                pool = _cluster->pool(i-1);
            } else if (r[i-1] == MEMCACHED_SUCCESS and r[i] == MEMCACHED_SUCCESS) {
                int prev = memcached_coll_result_get_count(&v[i-1]);
                int curr = memcached_coll_result_get_count(&v[i]);

                if (prev != curr) {
                    pool = _cluster->pool(i);
                } else {
                    continue;
                }
            }

            handle = pool->fetch(rc);

            if (handle) {
                r[i] = memcached_delete(handle, key.c_str(), key.length(), 0);

                if (r[i] != MEMCACHED_SUCCESS) {
                    ERROR("sync failed : memcached_delete (%s)", pool->to_string());
                }

                // TODO. Actually sync the data.
            } else {
                ERROR("sync failed : pool->fetch (%s)", pool->to_string());
            }

            pool->release(handle, NULL);
        }

        // free all other results excluding the value of master.
        for (int i = 0; i < v.size(); ++i) {
            if (result_index == i) continue;
            memcached_coll_result_free(&v[i]);
        }

        return (run_count >= 1);
#endif
    }

    bool base_client::insert(std::string key, const size_t num_of_items,
                             const uint64_t *bkeys, const char * const *values, const size_t *value_lengths,
                             memcached_coll_create_attrs_st *create_attrs, memcached_return_t *rc)
    {
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;
        memcached_return_t *results = (memcached_return_t *)malloc(sizeof(memcached_return_t)*num_of_items);
        memcached_return_t piped_rc;
 
        /* memcached_coll_create_attrs_set_expiretime(create_attrs, 1); */
        memcached_coll_create_attrs_set_unreadable(create_attrs, true);

        memcached_coll_attrs_st attrs; 
        memcached_coll_attrs_init(&attrs);
        memcached_coll_attrs_set_readable_on(&attrs);

        if (1) { /* _main_pool != NULL */
            handle = _main_pool->fetch(rc);
            do {
                *rc = memcached_bop_create(handle, key.c_str(), key.length(), create_attrs);
                if (*rc != MEMCACHED_SUCCESS) break;

                *rc = memcached_bop_piped_insert(handle, key.c_str(), key.length(), num_of_items,
                               bkeys, NULL, 0, values, value_lengths, NULL, results, &piped_rc);
                if (piped_rc != MEMCACHED_ALL_SUCCESS) break;

                *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);
                if (*rc != MEMCACHED_SUCCESS) break;
            } while(0);
            _main_pool->release(handle, NULL);
        }

        if ((_bkup_pool != NULL) && (*rc == MEMCACHED_SUCCESS || *rc == MEMCACHED_NO_SERVERS)) { 
            handle = _bkup_pool->fetch(rc);
            do {
                *rc = memcached_bop_create(handle, key.c_str(), key.length(), create_attrs);
                if (*rc != MEMCACHED_SUCCESS) break;

                *rc = memcached_bop_piped_insert(handle, key.c_str(), key.length(), num_of_items,
                               bkeys, NULL, 0, values, value_lengths, NULL, results, &piped_rc);
                if (piped_rc != MEMCACHED_ALL_SUCCESS) break;

                *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);
                if (*rc != MEMCACHED_SUCCESS) break;
            } while(0);
            _bkup_pool->release(handle, NULL);
        }

        free(results);
        return (*rc == MEMCACHED_SUCCESS);
#else
        int run_count = 0;
        memcached_return_t *results = (memcached_return_t *)malloc(sizeof(memcached_return_t)*num_of_items);
        memcached_return_t piped_rc;

        memcached_coll_create_attrs_set_unreadable(create_attrs, true);

        memcached_coll_attrs_st attrs; 
        memcached_coll_attrs_init(&attrs);
        memcached_coll_attrs_set_readable_on(&attrs);

        // insert the data into all cache clusters.
        for (int i = 0; i < _cluster->pool_size(); ++i) {
            connection_pool *pool = _cluster->pool(i); 

            memcached_st *handle = pool->fetch(rc);

            // get data by range. retry if itemout occurs. 
            for (int j = 0; handle, j < DEFAULT_OPERATION_RETRY_COUNT; ++j) {
                // remove existing key when retrying.
                if (j > 0) {
                    *rc = memcached_delete(handle, key.c_str(), key.length(), 0);
                    if (*rc != MEMCACHED_SUCCESS) {
                        ERROR("delete failed : %s (%s)", memcached_strerror(NULL, *rc), pool->to_string());
                    }
                }

                // create with unreadable state.
                *rc = memcached_bop_create(handle, key.c_str(), key.length(), create_attrs);

                if (*rc == MEMCACHED_TIMEOUT) continue;
                if (*rc != MEMCACHED_SUCCESS) break;

                *rc = memcached_bop_piped_insert(handle, key.c_str(), key.length(), num_of_items,
                        bkeys, NULL, 0, values, value_lengths, NULL, results, &piped_rc);

                if (*rc == MEMCACHED_TIMEOUT) continue;
                if (piped_rc != MEMCACHED_ALL_SUCCESS) break;

                // change to readable state 
                *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);

                if (*rc == MEMCACHED_SUCCESS) ++run_count;
                if (*rc != MEMCACHED_TIMEOUT) break;
            }

            // do rollback if an error occurs during data insertion.
            if (*rc != MEMCACHED_SUCCESS) {
                ERROR("insert failed : rolling back (%s)", pool->to_string());
                *rc = memcached_delete(handle, key.c_str(), key.length(), 0);
                if (*rc != MEMCACHED_SUCCESS) {
                    ERROR("delete failed : %s (%s)", memcached_strerror(NULL, *rc), pool->to_string());
                }
            }

            pool->release(handle, NULL);
        }

        free(results);

        return (run_count >= 1);
#endif
    }

    bool base_client::update(std::string key, const uint64_t bkey, const char *value, const size_t value_length,
                             memcached_return_t *rc)
    {
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;
 
        if (_bkup_pool != NULL) {
            handle = _bkup_pool->fetch(rc);
            *rc = memcached_bop_delete(handle, key.c_str(), key.length(), bkey, NULL, false);
            _bkup_pool->release(handle, NULL);
            if (*rc != MEMCACHED_SUCCESS) return false; 
        }
        if (1) { /* _main_pool != NULL */
            handle = _main_pool->fetch(rc);
            *rc = memcached_bop_update(handle, key.c_str(), key.length(), bkey, NULL, value, value_length);
            _main_pool->release(handle, NULL);
        }
        if ((_bkup_pool != NULL) && (*rc == MEMCACHED_SUCCESS || *rc == MEMCACHED_NO_SERVERS)) { 
            handle = _bkup_pool->fetch(rc);
            *rc = memcached_bop_insert(handle, key.c_str(), key.length(), bkey, NULL, 0, value, value_length, NULL);
            _bkup_pool->release(handle, NULL);
        }
        return (*rc == MEMCACHED_SUCCESS);
#else
        int run_count = 0;

        // update the data from all cache clusters.
        for (int i = 0; i < _cluster->pool_size(); ++i) {
            connection_pool *pool = _cluster->pool(i);

            memcached_st *handle = pool->fetch(rc);

            // update the data. retry if timeout occurs.
            for (int j = 0; handle, j < DEFAULT_OPERATION_RETRY_COUNT; ++j) {
                *rc = memcached_bop_update(handle, key.c_str(), key.length(), bkey, NULL, value, value_length);

                if (*rc == MEMCACHED_SUCCESS) ++run_count;
                if (*rc != MEMCACHED_TIMEOUT) break;
            }

            pool->release(handle, NULL);
        }

        return (run_count >= 1);
#endif
    }

    connection_pool *base_client::pool(size_t i)
    {
        return _cluster->pool(i);
    }

} // namespace arcus
