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
* @author harebox <harebox@nhn.com> junhyun park <junhyun.park@nhn.com>
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

#define ENABLE_TWO_COPY_REP 1 // junhyun

namespace arcus
{
    const size_t    MASTER_CLUSTER = 0;
    const size_t    BACKUP_CLUSTER = 1;

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

        if (_pool == NULL) {
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
        if (rc == NULL) {
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
        if (_pool != NULL) {
            arcus_pool_close(_pool);
            memcached_pool_destroy(_pool);
            _pool = NULL;
        }

        if (_master_handle != NULL) {
            memcached_free(_master_handle);
            _master_handle = NULL;
        }
    }

    /* CLUSTERS */

    clusters::clusters()
    {
    }

    clusters::~clusters()
    {
        for (int i=0; i<_pools.size(); i++) {
            connection_pool *pool = _pools[i];
            delete pool;
        }
        _pools.clear();
    }

    void clusters::add(std::string admin, std::string service_code, int32_t initial, int32_t max, uint64_t timeout)
    {
        connection_pool *pool = new connection_pool(admin, service_code, initial, max, timeout);
        _pools.push_back(pool);
        DEBUG("created a connection pool for the cluster (%s)", pool->to_string());
    }

    size_t clusters::size()
    {
        return _pools.size();
    }

    connection_pool *clusters::pool(size_t i)
    {
        return _pools[i];
    }

    /* CACHED_DATA */

    cached_data::cached_data()
        : _result(NULL)
    {
    }

    cached_data::cached_data(memcached_coll_result_st *result)
        : _result(result)
    {
    }

    cached_data::~cached_data()
    {
        if (_result != NULL) {
            memcached_coll_result_free(_result);
            _result = NULL;
        }
    }

    size_t cached_data::size()
    {
        return (_result != NULL) ? memcached_coll_result_get_count(_result) : 0;
    }

    const char *cached_data::value(size_t index)
    {
        return (_result != NULL and index < size()) ? memcached_coll_result_get_value(_result, index) : NULL;
    }

    size_t cached_data::value_length(size_t index)
    {
        return (_result != NULL and index < size()) ? memcached_coll_result_get_value_length(_result, index) : 0;
    }

    /* BASE_CLIENT */

    base_client::base_client()
    {
        _cluster = new clusters();
    }

    base_client::~base_client()
    {
        if (_cluster != NULL) {
            delete _cluster;
            _cluster = NULL;
        }
    }

    bool base_client::init(std::string admin, std::string service_codes, int32_t initial, int32_t max, uint64_t timeout)
    {
        std::stringstream stream(service_codes);
        std::string code;

        while (std::getline(stream, code, ',')) {
            _cluster->add(admin, code, initial, max, timeout);
        }

        // FIXME
#ifdef ENABLE_TWO_COPY_REP
        if (_cluster->size() < 1 or _cluster->size() > 2) {
            return false;
        }

        _master_pool = _cluster->pool(MASTER_CLUSTER);
        _backup_pool = _cluster->pool(BACKUP_CLUSTER);
        assert(_master_pool != NULL);
#else
        if (_cluster->size() != 1) {
            return false;
        }
#endif

        return true;
    }

    bool base_client::del(std::string key, memcached_return_t *rc)
    {
        bool result = false;
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;

        /* step 1: delete from backup cluster */
        if (_backup_pool != NULL) {
            if ((handle = _backup_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                return result;
            }
            for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                *rc = memcached_delete(handle, key.c_str(), key.length(), 0);
                if (*rc != MEMCACHED_TIMEOUT) break;
            }
            _backup_pool->release(handle, NULL);

            if (*rc == MEMCACHED_SUCCESS ||
                *rc == MEMCACHED_NOTFOUND || *rc == MEMCACHED_NO_SERVERS/*cloud failure*/) {
                /* Go downward in order to delete key in master cluster  */
            } else {
                return result;
            }
        }

        /* step 2: delete from master cluster */
        if (1) { /* _master_pool != NULL */
            if ((handle = _master_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                 return result;
            }
            for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                *rc = memcached_delete(handle, key.c_str(), key.length(), 0);
                if (*rc != MEMCACHED_TIMEOUT) break;
            }
            _master_pool->release(handle, NULL);

            if (*rc == MEMCACHED_SUCCESS || *rc == MEMCACHED_NOTFOUND) {
                result = true;
            }
        }
#else
        if (1) {
            connection_pool *pool = _cluster->pool(MASTER_CLUSTER); 
            memcached_st *handle = pool->fetch(rc);

            for (size_t i = 0; handle != NULL && i < DEFAULT_OPERATION_RETRY_COUNT; ++i) {
                *rc = memcached_delete(handle, key.c_str(), key.length(), 0);

                if (*rc == MEMCACHED_TIMEOUT) {
                    continue;
                }

                if (*rc == MEMCACHED_SUCCESS or
                    *rc == MEMCACHED_NOTFOUND) {
                    result = true;
                }
            }

            pool->release(handle, NULL);
        }
#endif

        return result;
    }  

    cached_data *base_client::get(std::string key, const uint64_t from, const uint64_t to, memcached_return_t *rc)
    {
        cached_data *result = NULL;
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;
        memcached_coll_result_st *coll_result;

        if (1) { /* _master_pool != NULL */
            if ((handle = _master_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                return result;
            }
            if ((coll_result = memcached_coll_result_create(handle, NULL)) == NULL) {
                /* *rc: how to set the error ? */
                *rc = MEMCACHED_MEMORY_ALLOCATION_FAILURE;
            } else {
                for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                    *rc = memcached_bop_get_by_range(handle, key.c_str(), key.length(), from, to,
                                                     NULL, 0, 0, false, false, coll_result);
                    if (*rc != MEMCACHED_TIMEOUT) break;
                }
                if (*rc == MEMCACHED_SUCCESS) {
                    result = new cached_data(coll_result);
                } else {
                    memcached_coll_result_free(coll_result);
                }
            }
            _master_pool->release(handle, NULL);
        }

        if (*rc == MEMCACHED_NOTFOUND || *rc == MEMCACHED_NO_SERVERS) {
            /* result == NULL */
            if (_backup_pool != NULL) {
                memcached_return_t brc;
                do {
                    if ((handle = _backup_pool->fetch(&brc)) == NULL) break;
                    if ((coll_result = memcached_coll_result_create(handle, NULL)) == NULL) {
                        /* *rc: how to set the error ? */
                        *rc = MEMCACHED_MEMORY_ALLOCATION_FAILURE; /* ?? */
                    } else {
                        for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                            brc = memcached_bop_get_by_range(handle, key.c_str(), key.length(), from, to,
                                                             NULL, 0, 0, false, false, coll_result);
                            if (brc != MEMCACHED_TIMEOUT) break;
                        }
                        if (brc == MEMCACHED_SUCCESS) {
                            result = new cached_data(coll_result);
                            *rc = brc;
                            /* resync to master pool (is it really needed ??) */
                        } else {
                            memcached_coll_result_free(coll_result);
                        }
                    } 
                    _backup_pool->release(handle, NULL);
                } while(0);
            }
        }
#else
        if (1) {
            connection_pool *pool = _cluster->pool(MASTER_CLUSTER); 
            memcached_st *handle = pool->fetch(rc);
            memcached_coll_result_st *coll_result = memcached_coll_result_create(handle, NULL);

            for (size_t i = 0; handle != NULL && i < DEFAULT_OPERATION_RETRY_COUNT; ++i) {
                *rc = memcached_bop_get_by_range(handle, key.c_str(), key.length(), from, to, NULL, 0, 0, false, false, coll_result);

                if (*rc != MEMCACHED_TIMEOUT) {
                    break;
                }
            }

            if (*rc == MEMCACHED_SUCCESS) {
                result = new cached_data(coll_result);
            } else {
                memcached_coll_result_free(coll_result);
            }

            pool->release(handle, NULL);
        }
#endif

        return result;
    }

    bool base_client::insert(std::string key, const size_t num_of_items,
                             const uint64_t *bkeys, const char * const *values, const size_t *value_lengths,
                             memcached_coll_create_attrs_st *create_attrs, memcached_return_t *rc)
    {
        bool result = false;
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;
        memcached_return_t *piped_results;
        memcached_return_t piped_rc;

        memcached_coll_attrs_st set_attrs; 
        memcached_coll_attrs_init(&set_attrs);
        memcached_coll_attrs_set_expiretime(&set_attrs, create_attrs->expiretime);
        memcached_coll_attrs_set_readable_on(&set_attrs);
        
        memcached_coll_create_attrs_set_expiretime(create_attrs, 2); /* expiretime = 2 */
        memcached_coll_create_attrs_set_unreadable(create_attrs, true);

        piped_results = (memcached_return_t*)malloc(sizeof(memcached_return_t)*num_of_items);
        if (piped_results == NULL) {
            *rc = MEMCACHED_MEMORY_ALLOCATION_FAILURE;
            return result;
        }

        if (1) { /* _master_pool != NULL */
            if ((handle = _master_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                free(piped_results);
                return result;
            }

            for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                /* 1. create with unreadable flag */ 
                *rc = memcached_bop_create(handle, key.c_str(), key.length(), create_attrs);
                if (*rc == MEMCACHED_TIMEOUT) {
                    /* get the count of elements */ 
                    memcached_coll_attrs_st get_attrs; 
                    *rc = memcached_get_attrs(handle, key.c_str(), key.length(), &get_attrs);
                    if (*rc == MEMCACHED_NOTFOUND) continue;
                    if (*rc == MEMCACHED_SUCCESS) {
                        if (get_attrs.count != 0) *rc = MEMCACHED_EXISTS;
                        /* attrs.count == 0: OK => go downward */ 
                    }
                }
                /* 2. insert columns(elements) */
                if (*rc == MEMCACHED_SUCCESS) {
                    *rc = memcached_bop_piped_insert(handle, key.c_str(), key.length(), num_of_items,
                                 bkeys, NULL, 0, values, value_lengths, NULL, piped_results, &piped_rc);
                    if (*rc == MEMCACHED_TIMEOUT) {
                        memcached_return_t rrc = memcached_delete(handle, key.c_str(), key.length(), 0);
                        if (rrc == MEMCACHED_SUCCESS || rrc == MEMCACHED_NOTFOUND) continue;
                    }
                }
                break;
            }

            if (*rc == MEMCACHED_SUCCESS && piped_rc == MEMCACHED_ALL_SUCCESS) {
                /* 3. set readable */
                for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                    *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &set_attrs);
                    if (*rc != MEMCACHED_TIMEOUT) break;
                }
                if (*rc == MEMCACHED_SUCCESS) {
                    result = true;
                }
            }

            if (result != true) {
                if (*rc == MEMCACHED_EXISTS) {
                    DEBUG("key(%s) or element exists (%s)", key.c_str(), _master_pool->to_string());
                } else {
                    ERROR("insert failed : rolling back (%s)", _master_pool->to_string());
                    memcached_return_t rrc = memcached_delete(handle, key.c_str(), key.length(), 0);
                    if (rrc != MEMCACHED_SUCCESS && rrc != MEMCACHED_NOTFOUND) {
                        ERROR("delete failed : %s (%s)", memcached_strerror(NULL, rrc), _master_pool->to_string());
                    } 
                }
            } 

            _master_pool->release(handle, NULL);
        } while(0);

        if (_backup_pool != NULL) {
            if (result == true || *rc == MEMCACHED_NO_SERVERS) {
                memcached_return_t brc;
                if ((handle = _backup_pool->fetch(&brc)) != NULL) {
                    do {
                        /* 1. create with unreadable flag */ 
                        brc = memcached_bop_create(handle, key.c_str(), key.length(), create_attrs);
                        if (brc != MEMCACHED_SUCCESS) break;

                        /* 2. insert columns(elements) */
                        brc = memcached_bop_piped_insert(handle, key.c_str(), key.length(), num_of_items,
                                     bkeys, NULL, 0, values, value_lengths, NULL, piped_results, &piped_rc);
                        if (brc != MEMCACHED_SUCCESS || piped_rc != MEMCACHED_ALL_SUCCESS) break;

                        /* 3. set readable */
                        brc = memcached_set_attrs(handle, key.c_str(), key.length(), &set_attrs);
                    } while(0);

                    if (brc != MEMCACHED_SUCCESS || piped_rc != MEMCACHED_ALL_SUCCESS) {
                        brc = memcached_delete(handle, key.c_str(), key.length(), 0);
                        if (brc != MEMCACHED_SUCCESS && brc != MEMCACHED_NOTFOUND) {
                            ERROR("delete failed : %s (%s)", memcached_strerror(NULL, brc), _backup_pool->to_string());
                        }
                    }
                    _backup_pool->release(handle, NULL);
                }
            }
        }
#else
        memcached_return_t *piped_results = (memcached_return_t*)malloc(sizeof(memcached_return_t)*num_of_items);
        memcached_return_t piped_rc;

        memcached_coll_create_attrs_set_unreadable(create_attrs, true);

        memcached_coll_attrs_st attrs; 
        memcached_coll_attrs_init(&attrs);
        memcached_coll_attrs_set_readable_on(&attrs);

        if (1) {
            connection_pool *pool = _cluster->pool(MASTER_CLUSTER); 
            memcached_st *handle = pool->fetch(rc);

            // FIXME
            for (size_t i = 0; handle != NULL && i < DEFAULT_OPERATION_RETRY_COUNT; ++i) {
                // 0. remove existing key when retry.    
                if (i > 0) {
                    DEBUG("insert : retry %d (%s)", i, pool->to_string());
                    memcached_return_t rrc = memcached_delete(handle, key.c_str(), key.length(), 0);
                    if (rrc != MEMCACHED_SUCCESS) {
                        ERROR("delete failed : %s (%s)", memcached_strerror(NULL, rrc), pool->to_string());
                        continue;
                    }
                }

                // 1. create with unreadable state.
                *rc = memcached_bop_create(handle, key.c_str(), key.length(), create_attrs);

                if (*rc == MEMCACHED_TIMEOUT) {
                    continue;
                }

                if (*rc != MEMCACHED_SUCCESS) {
                    break;
                }

                // 2. insert fields
                *rc = memcached_bop_piped_insert(handle, key.c_str(), key.length(), num_of_items,
                         bkeys, NULL, 0, values, value_lengths, NULL, piped_results, &piped_rc);

                if (*rc == MEMCACHED_TIMEOUT) {
                    continue;
                }

                if (piped_rc != MEMCACHED_ALL_SUCCESS) {
                    break;
                }

                // 3. change to readable state. 
                *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);

                if (*rc != MEMCACHED_TIMEOUT) {
                    break;
                }
            }

            if (*rc == MEMCACHED_SUCCESS) {
                result = true;
            } else if (*rc == MEMCACHED_EXISTS) {
                DEBUG("key(%s) or element exists (%s)", key.c_str(), pool->to_string());
            } else {
                ERROR("insert failed : rolling back (%s)", pool->to_string());
                memcached_return_t rrc = memcached_delete(handle, key.c_str(), key.length(), 0);
                if (rrc != MEMCACHED_SUCCESS) {
                    ERROR("delete failed : %s (%s)", memcached_strerror(NULL, rrc), pool->to_string());
                }
            }

            pool->release(handle, NULL);
        }
#endif

        free(piped_results);

        return result;
    }

    bool base_client::update(std::string key, const uint64_t bkey, const char *value, const size_t value_length,
                             memcached_return_t *rc)
    {
        bool result = false;
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;
        memcached_return_t brc;

        /* step 1: */
        if (_backup_pool != NULL) {
            if ((handle = _backup_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                return result;
            }
            for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                *rc = memcached_bop_delete(handle, key.c_str(), key.length(), bkey, NULL, false);
                if (*rc != MEMCACHED_TIMEOUT) break;
            }
            _master_pool->release(handle, NULL);

            if (*rc == MEMCACHED_SUCCESS || *rc == MEMCACHED_NO_SERVERS || *rc == MEMCACHED_NOTFOUND) {
                /* *rc == MEMCACHED_NOTFOUND_ELEMENT: how to handle it ? */
                /* Go downward  */
                brc = *rc; 
            } else {
                return result;
            } 
        }

        /* step 2: */
        if (1) { /* _master_pool != NULL */
            if ((handle = _master_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                return result;
            }
            for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                *rc = memcached_bop_update(handle, key.c_str(), key.length(), bkey, NULL, value, value_length);
                if (*rc != MEMCACHED_TIMEOUT) break;
            }
            _master_pool->release(handle, NULL);

            if (*rc == MEMCACHED_SUCCESS) {
                result = true;
            }
        } 

        /* step 3: */
        if (_backup_pool != NULL && brc == MEMCACHED_SUCCESS) {
            if (result == true || *rc == MEMCACHED_NO_SERVERS || *rc == MEMCACHED_NOTFOUND) {
                do { 
                    if ((handle = _backup_pool->fetch(&brc)) == NULL) break;
                    for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                        brc = memcached_bop_upsert(handle, key.c_str(), key.length(), bkey,
                                                    NULL, 0, value, value_length, NULL);
                        if (brc != MEMCACHED_TIMEOUT /* and brc != MEMCACHED_OUTOFMEMORY */) break;
                    }
                    _backup_pool->release(handle, NULL);

                    if (brc == MEMCACHED_SUCCESS && result == false) {
                        *rc = brc; result = true;
                        /* can resync to master pool */ 
                    }
                } while(0);
            }
        }
#else
        if (1) {
            connection_pool *pool = _cluster->pool(MASTER_CLUSTER);
            memcached_st *handle = pool->fetch(rc);

            for (size_t i = 0; handle != NULL && i < DEFAULT_OPERATION_RETRY_COUNT; ++i) {
                *rc = memcached_bop_update(handle, key.c_str(), key.length(), bkey, NULL, value, value_length);

                if (*rc != MEMCACHED_TIMEOUT) {
                    break;
                }
            }

            pool->release(handle, NULL);
        }

        if (*rc == MEMCACHED_SUCCESS) {
            result = true;
        }
#endif

        return result;
    }

    bool base_client::touch(std::string key, int32_t exptime, memcached_return_t *rc)
    {
        bool result = false;
#ifdef ENABLE_TWO_COPY_REP
        memcached_st *handle;
#endif

        memcached_coll_attrs_st attrs; 
        memcached_coll_attrs_init(&attrs);
        memcached_coll_attrs_set_expiretime(&attrs, exptime);

#ifdef ENABLE_TWO_COPY_REP
        if (1) { /* _master_pool != NULL */
            if ((handle = _master_pool->fetch(rc)) == NULL) {
                /* *rc: MEMCACHED_IN_PROGRESS || MEMCACHED_NOTFOUND || MEMCACHED_TIMEOUT ||
                        MEMCACHED_ERRNO || MEMCACHED_SUCCESS(grow_pool() failure) */
                return result;
            }
            for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);
                if (*rc != MEMCACHED_TIMEOUT) break;
            }
            _master_pool->release(handle, NULL);

            if (*rc == MEMCACHED_SUCCESS) {
                result = true;
            }
        }

        if (_backup_pool != NULL) {
            if (result == true || *rc == MEMCACHED_NOTFOUND || *rc == MEMCACHED_NO_SERVERS) {
                memcached_return_t brc;
                do {
                    if ((handle = _backup_pool->fetch(&brc)) == NULL) break;
                    for (int i = 0; i < DEFAULT_OPERATION_RETRY_COUNT; i++) {
                        brc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);
                        if (brc != MEMCACHED_TIMEOUT) break;
                    }
                    _backup_pool->release(handle, NULL);

                    if (brc == MEMCACHED_SUCCESS && result == false) {
                        *rc = brc; result = true;
                        /* can resync to master pool */
                    }
                } while(0);  
            }
        } 
#else
        if (1) {
            connection_pool *pool = _cluster->pool(MASTER_CLUSTER);
            memcached_st *handle = pool->fetch(rc);

            for (size_t i = 0; handle != NULL && i < DEFAULT_OPERATION_RETRY_COUNT; ++i) {
                *rc = memcached_set_attrs(handle, key.c_str(), key.length(), &attrs);

                if (*rc != MEMCACHED_TIMEOUT) {
                    break;
                }
            }

            pool->release(handle, NULL);
        }

        if (*rc == MEMCACHED_SUCCESS) {
            result = true;
        }
#endif

        return result;
    }

#ifdef ENABLE_TWO_COPY_REP
#else
    connection_pool *base_client::pool(size_t i)
    {
        return _cluster->pool(i);
    }
#endif


} // namespace arcus
