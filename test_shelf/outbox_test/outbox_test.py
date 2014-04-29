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
import time
import sys
import random

#max_num_usrs = 10000000
max_num_usrs = 1000000
max_num_frds = 200000  
avg_num_frds = 10
min_num_frds = 0

relations = []
tot_relations = 0

def getFriendCount():
    frd_cls = random.randrange(0, 1000)
    if (frd_cls < 300):
        frd_cnt = random.randrange(0,10)
    elif (frd_cls < 700):
        frd_cnt = random.randrange(10,100)
    elif (frd_cls < 900):
        frd_cnt = random.randrange(100,300)
    elif (frd_cls < 950):
        frd_cnt = random.randrange(300,500)
    elif (frd_cls < 980):
        frd_cnt = random.randrange(500,1000)
    elif (frd_cls < 998):
        frd_cnt = random.randrange(1000,10000)
    else:
        frd_cnt = random.randrange(10000,max_num_frds)
    return frd_cnt

def buildFriendSet():
    for uid in range(0, max_num_usrs):
        frd_cnt = getFriendCount()
        frd_set = set([]) 
        while (len(frd_set) < frd_cnt):
           fid = random.randrange(0, max_num_usrs)
           if (fid != uid):
               frd_set.add(fid)
        relations.append(frd_set)
        tot_relations = len(frd_set)
        if ((uid % 1000) == 999):
            print str(uid+1) + "users created"
    print "total relations = " + str(tot_relations)

def main():
    buildFriendSet()
    
         
#    for uid in range(0, max_num_usrs):
#    max_split_cnt = 32
#    cur_split_cnt = 1
#    while (cur_split_cnt < max_split_cnt):
#        uid_range = max_num_usrs/cur_split_cnt
#        for range_idx range (0, cur_split_cnt)
#            s_uid = range_idx * uid_range
#            e_uid = s_uid + uid_range
#            for i range (s_uid, e_uid)
#                tot
#        
#        
#
#        cur_split_cnt = cur_split_cnt * 2
#
#    for uid in range(0, 10):
#        print relations[uid], len(relations[uid])
#         
#        for cls in range(0, 100)
#        relations[uid][uid]
        

if __name__ == '__main__':
    main()

