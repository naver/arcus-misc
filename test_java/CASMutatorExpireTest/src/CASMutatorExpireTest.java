import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;

import net.spy.memcached.CASMutator;
import net.spy.memcached.CASMutation;
import net.spy.memcached.internal.CollectionFuture;
import net.spy.memcached.collection.CollectionAttributes;;
import net.spy.memcached.transcoders.SerializingTranscoder;

public class CASMutatorExpireTest extends ArcusTest {

	public static void main(String[] args) {
		ArcusTest test = new CASMutatorExpireTest();
		if (args.length > 0)
			test.parseArgs(args);

		System.out.println("CAS Mutator test start.");
		test.beforeTest();
		try {
			test.testStart();
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			test.afterTest();
		}
		System.out.println("CAS Mutator test end.");
	}

	public void testStart() throws Exception {
        String testKey = "TEST_PREFIX:TEST_KEY";
        String testValue = "TEST_VALUE";
        Integer expireTime = 4; // 4초

        arcusClient.delete(testKey); // 아커스 내 test value clear

        CASMutation<String> mutation = new CASMutation<String>() {
            @Override
                public String getNewValue(String current) {
                    return current;
                }
        };

        CASMutator<String> mutator = new CASMutator(arcusClient, new SerializingTranscoder());

        mutator.cas(testKey, testValue, expireTime, mutation); //값이 없으므로 add
        mutator.cas(testKey, testValue, expireTime, mutation); // 다시 call하여 update 시킨다, 값이 살아있을 때 업데이트 되는 상황

        CollectionFuture<CollectionAttributes> attrs = arcusClient.asyncGetAttr(testKey);

        Thread.sleep(6000); // 6초후 수행 expire time 초과시킨다

        String result = (String)arcusClient.get(testKey);
        System.out.println(attrs.get().getExpireTime());
        System.out.println(result);
	}
}
