import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import net.spy.memcached.ArcusClient;
import net.spy.memcached.ConnectionFactoryBuilder;
import net.spy.memcached.collection.CollectionAttributes;
import net.spy.memcached.collection.CollectionOverflowAction;
import net.spy.memcached.collection.CollectionResponse;
import net.spy.memcached.collection.Element;
import net.spy.memcached.collection.ElementFlagFilter;
import net.spy.memcached.collection.ElementValueType;
import net.spy.memcached.internal.CollectionFuture;
import net.spy.memcached.ops.CollectionOperationStatus;

public class AsyncBopPipedInsertBulkTest extends ArcusTest {

	public static void main(String[] args) {
		ArcusTest test = new AsyncBopPipedInsertBulkTest();
		if (args.length > 0)
			test.parseArgs(args);

		System.out.println("asyncBopPipedInsertBulk test start.");
		test.beforeTest();
		try {
			test.testStart();
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			test.afterTest();
		}
		System.out.println("asyncBopPipedInsertBulk test end.");
	}

	public void testStart() throws Exception {
		int bopPipedBulkInsertMax = 20 /* 0 ~ 19 */;
		int expireTime = 2;
		long btreeSize = 10;
		CollectionOverflowAction overflowAction = CollectionOverflowAction.smallest_trim;
		CollectionAttributes attributes = new CollectionAttributes(expireTime,
				btreeSize, overflowAction);

		String key = KEY_PREFIX + ":" + "BTreePipedBulkInsert";

		ElementValueType vtype = ElementValueType.STRING;

		/* create btree for bopPipedInsertBulk test */
		System.out.println("Create b+tree.");
		CollectionFuture<Boolean> createFuture = null;
		createFuture = arcusClient.asyncBopCreate(key, vtype, attributes);

		assert createFuture.get() == true :
			"b+tree creation failed. because of " + createFuture.getOperationStatus().getResponse();
		System.out.printf("RESULT : %s. OK!\n\n", createFuture.getOperationStatus().getMessage());

		List<Element<Object>> elements = new ArrayList<Element<Object>>();
		byte[] eflag = { 0 };

		/* create bopPipedInsertBulk elements 0 ~ 19 */
		System.out.printf("Create %s elements. 0 ~ %s.\n", bopPipedBulkInsertMax, bopPipedBulkInsertMax - 1);
		for (int bkey = 0; bkey < bopPipedBulkInsertMax; bkey++)
			elements.add(new Element<Object>((long) bkey, "VALUE" + bkey, eflag));

		/* must element size less than getMaxPiepedItemCount */
		assert elements.size() <= arcusClient.getMaxPipedItemCount() :
			"Too many elements. getMaxPipedItemCount is " + arcusClient.getMaxPipedItemCount()
			+ ". but element count is " + elements.size();
		System.out.printf("RESULT : %d <= MaxPipedItemCount(%d). OK!\n\n", elements.size(), arcusClient.getMaxPipedItemCount());

		/* insert element by bopPipedInsertBulk API */
		System.out.printf("Piped insert %d elements. 0 ~ %d.\n", bopPipedBulkInsertMax, bopPipedBulkInsertMax - 1);
		CollectionFuture<Map<Integer, CollectionOperationStatus>> insertFuture = null;
		insertFuture = arcusClient.asyncBopPipedInsertBulk(key, elements, attributes);

		Map<Integer, CollectionOperationStatus> insertResult = insertFuture.get();

		/* must all element are inserted, but trimmed 0 ~ 9 */
		assert insertResult.isEmpty() == true : "insertion result must is all ok. but " + insertResult.toString();
		System.out.printf("RESULT : fail count %d. OK!\n\n", insertResult.size());
		
		/* get element test */
		CollectionFuture<Map<Long, Element<Object>>> getFuture = null;
		Map<Long, Element<Object>> getResult = null;

		/* bkey is 0 */
		getFuture = arcusClient.asyncBopGet(key, (long) 0, null, false, false);

		/* element, bkey is 0, is trimmed. response is OUT_OF_RANGE */
		getResult = getFuture.get();
		System.out.printf("Get element(bkey 0) and check reponse.\n");
		assert getFuture.getOperationStatus().getResponse() == CollectionResponse.OUT_OF_RANGE :
				"Response(bkey 0) is must OUT_OF_RANGE! "
				+ "but Return Response is "
				+ getFuture.getOperationStatus().getMessage();
		System.out.printf("RESULT : %s. OK!\n\n", getFuture.getOperationStatus().getResponse());

		/* get elements that bkey is 20 */
		getFuture = arcusClient.asyncBopGet(key, (long) bopPipedBulkInsertMax, null, false, false);

		/* element, bkey is 20, don't be inserted. NOT_FOUND_ELEMENT */
		getResult = getFuture.get();
		System.out.printf("Get element(bkey %d) and check reponse.\n", bopPipedBulkInsertMax);
		assert getFuture.getOperationStatus().getResponse() == CollectionResponse.NOT_FOUND_ELEMENT : 
				"Response(bkey "
				+ (long) bopPipedBulkInsertMax
				+ ") is must NOT_FOUND_ELEMENT! "
				+ "but Return Reponse is "
				+ getFuture.getOperationStatus().getResponse();
		System.out.printf("RESULT : %s. OK!\n\n", getFuture.getOperationStatus().getResponse());

		/* element & b+tree clear */
		System.out.println("b+tree and element all clear. because of reinsertion.");
		elements.clear();
		arcusClient.delete(key);

		System.out.println("Recreate b+tree.");
		createFuture = arcusClient.asyncBopCreate(key, vtype, attributes);

		assert createFuture.get() == true :
			"b+tree creation failed. because of " + createFuture.getOperationStatus().getResponse();
		// arcusClient.asyncBopDelete(key, 0, bopPipedBulkInsertMax, ElementFlagFilter.DO_NOT_FILTER, 0, false);
		System.out.printf("RESULT : %s. OK!\n\n", createFuture.getOperationStatus().getResponse());

		/* create bopPipedInsertBulk elements 19 ~ 0 */
		System.out.printf("Create %d elements. %d ~ 0.\n", bopPipedBulkInsertMax, bopPipedBulkInsertMax - 1);
		for (int bkey = bopPipedBulkInsertMax - 1; bkey >= 0; bkey--)
			elements.add(new Element<Object>((long) bkey, "VALUE" + bkey, eflag));

		/* must element size less than getMaxPiepedItemCount */
		assert elements.size() <= arcusClient.getMaxPipedItemCount() : createFuture.getOperationStatus().getResponse();
		System.out.printf("RESULT : %d <= MaxPipedItemCount(%d). OK!\n\n", elements.size(), arcusClient.getMaxPipedItemCount());

		/* insert element by bopPipedInsertBulk API */
		System.out.printf("Piped insert %d elements. %d ~ 0.\n", bopPipedBulkInsertMax, bopPipedBulkInsertMax - 1);
		insertFuture = arcusClient.asyncBopPipedInsertBulk(key, elements, attributes);
		insertResult = insertFuture.get();

		assert insertResult.size() == bopPipedBulkInsertMax - btreeSize : 
				"Expected insertion fail count is "
				+ (bopPipedBulkInsertMax - btreeSize)
				+ ". but return fail count is " + insertResult.size();
		System.out.printf("RESULT : fail count %d. OK!\n\n", insertResult.size());

		/* 9 ~ 0 element don't be inserted. failed. There are trimmed */
		System.out.printf("Check insertion fail result %d elements. %d ~ 0.\n", btreeSize, btreeSize - 1);
		for (Map.Entry<Integer, CollectionOperationStatus> entry : insertResult.entrySet()) {
			assert entry.getValue().getResponse() == CollectionResponse.OUT_OF_RANGE : 
				entry.getKey() + " bkey response must be OUT_OF_RANGE. " 
				+ "but return response is " + entry.getValue().getResponse();
			System.out.printf("RESULT : bkey %d | %s\n", entry.getKey(), entry.getValue());
		}
		System.out.printf("OK!\n\n");
		
		/* get element test bkey is 0 */
		getFuture = arcusClient.asyncBopGet(key, (long) 0, null, false, false);

		/* element, bkey is 0, is trimmed. response is NOT_FOUND_ELEMENT */
		getResult = getFuture.get();
		System.out.printf("Get element(bkey 0) and check reponse.\n");
		assert getFuture.getOperationStatus().getResponse() == CollectionResponse.NOT_FOUND_ELEMENT :
				"Response(bkey 0) is must NOT_FOUND_ELEMENT! "
				+ "but Return Response is "
				+ getFuture.getOperationStatus().getMessage();
		System.out.printf("RESULT : %s. OK!\n\n", getFuture.getOperationStatus().getResponse());

		/* get elements that bkey is 20 */
		getFuture = arcusClient.asyncBopGet(key, (long) bopPipedBulkInsertMax, null, false, false);

		/* element, bkey is 20, don't be inserted. NOT_FOUND_ELEMENT */
		getResult = getFuture.get();
		System.out.printf("Get element(bkey %d) and check reponse.\n", bopPipedBulkInsertMax);
		assert getFuture.getOperationStatus().getResponse() == CollectionResponse.NOT_FOUND_ELEMENT : 
				"Response(bkey "
				+ (long) bopPipedBulkInsertMax
				+ ") is must NOT_FOUND_ELEMENT! "
				+ "but Return Reponse is "
				+ getFuture.getOperationStatus().getResponse();
		System.out.printf("RESULT : %s. OK!\n\n", getFuture.getOperationStatus().getResponse());
	}
}
