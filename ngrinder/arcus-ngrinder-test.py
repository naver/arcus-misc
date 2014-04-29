# -*- coding: utf-8 -*-
"""
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
"""
################################################
# Arcus workload simuation script for nGrinder #
################################################

from net.grinder.script import Test
from net.grinder.script.Grinder import grinder
from net.grinder.plugin.http import HTTPRequest, HTTPPluginControl

from net.spy.memcached import ArcusClient, ConnectionFactoryBuilder, FailureMode, AddrUtil
from net.spy.memcached.ops import OperationException
from net.spy.memcached.collection import CollectionAttributes, CollectionOverflowAction, Element, ElementValueType, CollectionAttributes, ElementFlagFilter, ElementFlagUpdate

from org.apache.log4j import Logger, Level;

from java.math import BigInteger
from java.lang import Long, Thread, Object, String, System
from java.util import ArrayList, HashMap, Random
from java.util.concurrent import TimeUnit, TimeoutException, ExecutionException, Future

import sys, random, string, time

######################################################
# GLOBAL SETTINGS
######################################################
# configurations for Arcus cloud
arcus_cloud = "127.0.0.1:2181"
service_codes = ["test"]
service_code = random.choice(service_codes)

USE_GLOBAL_CLIENT = False   # False=per thread , True=process

DEFAULT_CONNECTION_WAIT = 2 # init waiting
DEFAULT_TIMEOUT = 3000      # Operation default TO
DEFAULT_PREFIX = 'arcustest-'

# create a global client
if USE_GLOBAL_CLIENT:
    global_cfb = ConnectionFactoryBuilder()
    global_client = ArcusClient.createArcusClient(arcus_cloud, service_code, global_cfb)

    # wait for the client to be connected to Arcus cloud
    print 'Wait for global client to be connected to Arcus cloud (%d seconds)' % (DEFAULT_CONNECTION_WAIT)
    Thread.currentThread().sleep(DEFAULT_CONNECTION_WAIT * 1000)

######################################################
# WORKLOAD FUNCTIONS
######################################################
print 'creating workloads'
KeyLen = 20
ExpireTime = 600    # seconds
chunk_sizes = [96, 120, 152, 192, 240, 304, 384, 480, 600, 752, 944, 1184, 1480, 1856, 2320, 2904, 3632, 4544, 5680, 7104, 8880, 11104, 13880, 17352, 21696, 27120, 33904, 42384, 52984, 66232, 82792, 103496, 129376, 161720, 202152, 252696, 315872, 394840, 493552, 1048576]
chunk_values = ["Not_a_slab_class"]
for s in chunk_sizes:
    value = "".join([random.choice(string.lowercase) for i in range(int(s*2/3))])
    chunk_values.append(value)
    #chunk_values.append('empty')

dummystring = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijlmnopqrstuvwxyz"
def generateData(length):                                                                                                     
        ret = ''                                                                                                              
        for loop in range(length):                                                                                            
                randomInt = random.randint(0, 60)                                                                             
                tempchar = dummystring[randomInt]                                                                             
                ret = ret + tempchar                                                                                          
        return ret

workloads = [chunk_values[1], 
             chunk_values[1], 
             chunk_values[2], 
             chunk_values[2], 
             chunk_values[3]]

######################################################
# UTILITY FUNCTIONS
######################################################
def gen_key(name="unknown"):
    """
    Generates a key with given name and postfix
    """
    prefix = DEFAULT_PREFIX
    key = generateData(KeyLen);
    return "%s%s:%s" % (prefix, name, key)
   
def gen_workload(is_collection):
    """
    Generates a string workload with specific size.
    """
    if is_collection:
        return random.choice(chunk_values[0:17])
    else:
        return random.choice(chunk_values)

######################################################
# GRINDER TEST RUNNER
######################################################
class TestRunner:
    def __init__(self):
        """
        Initialize tests
        """
        Logger.getLogger("net.spy.memcached").setLevel(Level.DEBUG);
        self.clients = []
        if USE_GLOBAL_CLIENT:
            # use global client
            self.client = global_client
        else:
            cfb = ConnectionFactoryBuilder()
            self.client = ArcusClient.createArcusClient(arcus_cloud, service_code, cfb)

        print 'Wait for per-thread client to be connected to Arcus cloud (%d seconds)' % DEFAULT_CONNECTION_WAIT
        Thread.currentThread().sleep(DEFAULT_CONNECTION_WAIT * 1000)
        
        self.flush_counter = 0

        self.tests = []
       
        # insert operations
        self.tests.append(Test(1, "KeyValue").wrap(self.KeyValue))
        self.tests.append(Test(2, "Collection_Btree").wrap(self.Collection_Btree))
        self.tests.append(Test(3, "Collection_Set").wrap(self.Collection_Set))
        self.tests.append(Test(4, "Collection_List").wrap(self.Collection_List))

    def __call__(self):
        """
        Tests to run
        """
        for test in self.tests:
            test(gen_key(test.__name__))


    def arcusGet(self, future, name='unknown', timeout=DEFAULT_TIMEOUT, timeunit=TimeUnit.MILLISECONDS):
        try:
            return future.get(timeout, timeunit)
        except TimeoutException, e:
            print "TimeoutException(%s) : %s"%(name, str(e))
            future.cancel(1)
        except ExecutionException, e:
            print "ExecutionException(%s) : %s"%(name, str(e))
            try:
	        Thread.sleep(100)
            except Exception, e:
                pass

            future.cancel(1)
        except Exception, e:
            print "Exception : " + str(e)
            future.cancel(1)
        return None

######################################################
# TEST CASES
######################################################
    def KeyValue(self, key):
        """
        get:set:delete:incr:decr = 3:1:0.01:0.1:0.0001
        """
        name = 'KeyValue'
        workloads = [chunk_values[24]]
        
        # Set 
        for i in range(0, 1):
            future = self.client.set(key, ExpireTime, random.choice(workloads))
            result = self.arcusGet(future, name=name)
            #print str(result)
        
        # Get
        for i in range(0, 5):
            future = self.client.asyncGet(key)
            result = self.arcusGet(future, name=name)
            
        # Delete
        if random.randint(0, 3) == 0:
            future = self.client.delete(key)
            result = self.arcusGet(future, name=name)
            
        # Incr
        if random.randint(0, 1) == 0:
            future = self.client.set(key + 'numeric', ExpireTime, '1')
            result = self.arcusGet(future, name=name)
            future = self.client.asyncIncr(key + 'numeric', 1)
            result = self.arcusGet(future, name=name)
            
        # Decr
        if random.randint(0, 1) == 0:
            future = self.client.set(key + 'numeric', ExpireTime, '1')
            result = self.arcusGet(future, name=name)
            future = self.client.asyncDecr(key + 'numeric', 1)
            result = self.arcusGet(future, name=name)

    def Collection_Btree(self, key):
        """
        """
        name = 'Collection_Btree'
        keyList = []
        for i in range(4):                    # Create 5 key
            keyList.append(key + str(i))
    
        bkeyBASE = "bkey_byteArry"

        eflag = String("EFLAG").getBytes()
        filter = ElementFlagFilter(ElementFlagFilter.CompOperands.Equal, String("EFLAG").getBytes()) 
        attr = CollectionAttributes()
        attr.setExpireTime(ExpireTime)
        
        # BopInsert + byte_array bkey
        for j in range(4):                        # 5 Key 
            for i in range(50):                   # Insert 50 bkey
                bk = bkeyBASE + str(j) + str(i)   # Uniq bkey
                bkey = String(String.valueOf(bk)).getBytes()                        ####____####
                future = self.client.asyncBopInsert(keyList[j], bkey, eflag, random.choice(workloads), attr)
                result = self.arcusGet(future, name=name)
                #print str(result)

        # Bop Bulk Insert (Piped Insert)
        elements = []
        for i in range(50):
            bk = bkeyBASE + str(0) + str(i) + "bulk"
            elements.append(Element(String(str(bk)).getBytes(), workloads[0], eflag)) ####____####
        future = self.client.asyncBopPipedInsertBulk(keyList[0], elements, CollectionAttributes())
        result = self.arcusGet(future, name=name)
        #print str(result)
        
        # BopGet Range + filter
        for j in range(4):                    
            bk = bkeyBASE + str(j) + str(0)  
            bk_to = bkeyBASE + str(j) + str(50) 
            bkey = String(String.valueOf(bk)).getBytes()
            bkey_to = String(String.valueOf(bk_to)).getBytes()           ####____####
            future = self.client.asyncBopGet(keyList[j], bkey, bkey_to, filter, 0, random.randint(20, 50), False, False)
            result = self.arcusGet(future, name=name)
            #print str(result)
        
	# BopGetBulk  // 20120319 Ad
        bk = bkeyBASE + str(0) + str(0)                                                                                                                                      
        bk_to = bkeyBASE + str(4) + str(50)                                                                                                                                  
        bkey = String(String.valueOf(bk)).getBytes()                                                                                                                         
        bkey_to = String(String.valueOf(bk_to)).getBytes()           ####____####                                                                                            
        future = self.client.asyncBopGetBulk(keyList, bkey, bkey_to, filter, 0, random.randint(20, 50))                                                         
        result = self.arcusGet(future, name=name)
	#for entry in result.entrySet():
	#	print str(entry.getKey())

	#	if entry.getValue().getElements() is not None:
	#		print "["
	#		for element in entry.getValue().getElements().entrySet():
	#			print "bkey=%s, value=%s" % (str(element.getKey()), str(element.getValue().getValue()))
	#		print "]"
	#	else:
	#		print "[elements=%s, response=%s]" % (entry.getValue().getElements(), entry.getValue().getCollectionResponse().getMessage())
	#print ""
	#print str(result)
	
        # BopEmpty Create
        future = self.client.asyncBopCreate(key, ElementValueType.STRING, CollectionAttributes())
        result = self.arcusGet(future, name=name)
        #print str(result)

        # BopSMGet
        bk = bkeyBASE + str(0) + str(0)  
        bk_to = bkeyBASE + str(4) + str(50) 
        bkey = String(String.valueOf(bk)).getBytes()             ####____####
        bkey_to = String(String.valueOf(bk_to)).getBytes()       ####____####
        future = self.client.asyncBopSortMergeGet(keyList, bkey, bkey_to, filter, 0, random.randint(20, 50))
        result = self.arcusGet(future, name=name)
        #print str(result)
        
        # BopUpdate  (eflag bitOP + value)
        key = keyList[0]
        eflagOffset = 0
        value = "ThisIsChangeValue"
        bitop = ElementFlagUpdate(eflagOffset, ElementFlagFilter.BitWiseOperands.AND, String("aflag").getBytes())
        for i in range(2):                      # 3 element update
            bk = bkeyBASE + str(0) + str(i)
            bkey = String(String.valueOf(bk)).getBytes()       ####____####
            future = self.client.asyncBopUpdate(key, bkey, bitop, value)       
            result = self.arcusGet(future, name=name)
            #print str(result)
        
        # SetAttr  (change Expire Time)
        attr.setExpireTime(100)
        future = self.client.asyncSetAttr(key, attr)
        result = self.arcusGet(future, name=name)
        #print str(result)

        # BopDelete          (eflag filter delete)
        for j in range(4):                     
            bk = bkeyBASE + str(j) + str(0)  
            bk_to = bkeyBASE + str(j) + str(10) 
            bkey = String(String.valueOf(bk)).getBytes() ####____####
            bkey_to = String(String.valueOf(bk_to)).getBytes()  ####____####
            future = self.client.asyncBopDelete(keyList[j], bkey, bkey_to, filter, 0, False)
            result = self.arcusGet(future, name=name)
            #print str(result)
        
    def Collection_Set(self, key):
        """
        """
        name = 'Collection_Set'
        attr = CollectionAttributes()
        attr.setExpireTime(ExpireTime)
        keyList = []
        for i in range(4):                  
            keyList.append(key + str(i))

        # SopInsert
        for i in range(4):
            for j in range(19):
                set_value = workloads[i] + str(j)
                future = self.client.asyncSopInsert(keyList[i], set_value, attr)
                result = self.arcusGet(future, name=name)
                #print str(result)

        # SopInsert Bulk (Piped)
        elements = []
        for i in range(50):
            elements.append(str(i) + "_" + workloads[0])                                                                                               
        future = self.client.asyncSopPipedInsertBulk(key, elements, CollectionAttributes())                                                 
        result = self.arcusGet(future, name=name)                                                                                                      
        #print str(result)            

        # SopEmpty Create
        future = self.client.asyncSopCreate(key, ElementValueType.STRING, CollectionAttributes())
        result = self.arcusGet(future, name=name)
        #print str(result)

        # SopExist    (Piped exist)
        for i in range(4):
            listValue = []                                                                                                                             
            for j in range(9):   # 10 value exsist
                listValue.append(workloads[i] + str(j))
            future = self.client.asyncSopPipedExistBulk(keyList[i], listValue)
            result = self.arcusGet(future, name=name)
            #print str(result)

        # SetAttr  (change Expire Time)
        attr.setExpireTime(100)
        future = self.client.asyncSetAttr(keyList[0], attr)
        result = self.arcusGet(future, name=name)
        #print str(result)
         
        # SopDelete
        for i in range(4):
            for j in range(4):   # 5 element value delete
                delValue = workloads[i] + str(j)
                future = self.client.asyncSopDelete(keyList[i], delValue, True)
                result = self.arcusGet(future, name=name)
                #print str(result)

    def Collection_List(self, key):
        """
        """
        name = 'Collection_List'
        attr = CollectionAttributes()
        attr.setExpireTime(ExpireTime)
        keyList = []
        index = -1             # Tail insert
        for i in range(4):                  
            keyList.append(key + str(i))

        # LopInsert
        for i in range(4):
            for j in range(50):
                future = self.client.asyncLopInsert(keyList[i], index, random.choice(workloads), attr)
                result = self.arcusGet(future, name=name)
                #print str(result)

        # LopInsert Bulk (Piped)
        elements = []
        for i in range(50):
            elements.append(str(i) + "_" + workloads[0])
        future = self.client.asyncLopPipedInsertBulk(keyList[0], -1, elements, CollectionAttributes())
        result = self.arcusGet(future, name=name)
        #print str(result) 
       
        # LopGet
        for i in range(4):
            index = 0
            index_to = index + random.randint(20, 50)
            future = self.client.asyncLopGet(keyList[i], index, index_to, False, False)
            result = self.arcusGet(future, name=name)
            #print str(result)   

        # LopAttr
        attr.setExpireTime(100)                                                                                                                     
        future = self.client.asyncSetAttr(keyList[0], attr)                                                                                            
        result = self.arcusGet(future, name=name)                                                                                                      
        #print str(result)        

        # LopDelete
        index = 0
        index_to = 19
        for i in range(1):
            future = self.client.asyncLopDelete(keyList[i], index, index_to, True)
            result = self.arcusGet(future, name=name)
            #print str(result)

