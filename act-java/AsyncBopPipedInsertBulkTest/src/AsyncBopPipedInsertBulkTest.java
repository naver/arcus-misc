import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;

import net.spy.memcached.collection.CollectionAttributes;
import net.spy.memcached.collection.CollectionOverflowAction;
import net.spy.memcached.collection.CollectionResponse;
import net.spy.memcached.collection.Element;
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
		int bopPipedBulkInsertMax = 19 /* 0 ~ 19 */;
		int expireTime = 2;
		long btreeSize = 10;
		CollectionOverflowAction overflowAction = CollectionOverflowAction.smallest_trim;
		CollectionAttributes attributes = new CollectionAttributes(expireTime,
				btreeSize, overflowAction);

		String key = KEY_PREFIX + ":" + "BTreePipedInsertBulk";

		ElementValueType vtype = ElementValueType.STRING;

		/* create btree for bopPipedInsertBulk test */
		createBTree(key, vtype, attributes);
		
		/* 
		 * create bopPipedInsertBulk elements 0 ~ 19
		 * must element size less than getMaxPiepedItemCount 
		 */
		List<Element<Object>> elements = createElements(0, bopPipedBulkInsertMax);

		
		/* insert element by bopPipedInsertBulk API
		 * must all element are inserted, but trimmed 0 ~ 9 
		 */
		checkPipedInsert(0, key, elements);

		
		/* bkey is 0 element, 
		 * trimmed. response is OUT_OF_RANGE 
		 */
		checkBkey(key, 0, CollectionResponse.OUT_OF_RANGE);

		
		/* get elements that bkey is 20 
		 * don't be inserted. NOT_FOUND_ELEMENT
		 */
		checkBkey(key, (long)bopPipedBulkInsertMax + 1, CollectionResponse.NOT_FOUND_ELEMENT);

		
		/* getattr b+ tree */
		checkAttr(key);
		
		
		/* element & b+tree clear */
		System.out.println("b+tree and element all clear. because of reinsertion.");
		elements.clear();
		arcusClient.delete(key);

		/* recreate btree */
		createBTree(key, vtype, attributes);

		
		/* create bopPipedInsertBulk elements 19 ~ 0 
		 * must element size less than getMaxPiepedItemCount 
		 */
		elements = createElements(bopPipedBulkInsertMax, 0);
		
		/* insert element by bopPipedInsertBulk API */
		checkPipedInsert(bopPipedBulkInsertMax - (int)btreeSize + 1, key, elements);

		
		/* get element test bkey is 0
		 * is trimmed. response is NOT_FOUND_ELEMENT 
		 */
		checkBkey(key, 0, CollectionResponse.NOT_FOUND_ELEMENT, CollectionResponse.OUT_OF_RANGE);

		
		/* get elements that bkey is 20
		 * don't be inserted. NOT_FOUND_ELEMENT 
		 */
		checkBkey(key, (long)bopPipedBulkInsertMax + 1, CollectionResponse.NOT_FOUND_ELEMENT);
		
		/* getattr b+ tree */
		checkAttr(key);
	}

	private void createBTree(String key, ElementValueType vtype, CollectionAttributes attributes)
		throws InterruptedException, ExecutionException {
		CollectionFuture<Boolean> createFuture = null;
		
		System.out.println("Create b+tree.");
		createFuture = arcusClient.asyncBopCreate(key, vtype, attributes);

		assert createFuture.get() == true :
			"b+tree creation failed. because of " 
			+ createFuture.getOperationStatus().getResponse();

		System.out.printf("RESULT : %s. OK!\n\n",
						  createFuture.getOperationStatus().getMessage());
	}
	
	private List<Element<Object>> createElements(int from, int to) {
		List<Element<Object>> elements = new ArrayList<Element<Object>>();
		byte[] eflag = { 0 };

		// System.out.printf("Create %d ~ %d elements.\n", from, to);
		if (from <= to) {
			for (int bkey = from; bkey <= to; bkey++)
				elements.add(new Element<Object>((long) bkey, "VALUE" + bkey, eflag));
		} else {
			for (int bkey = from; bkey >= to; bkey--)
				elements.add(new Element<Object>((long) bkey, "VALUE" + bkey, eflag));
		}

		assert elements.size() <= arcusClient.getMaxPipedItemCount() :
			"Too many elements. getMaxPipedItemCount is "
			+ arcusClient.getMaxPipedItemCount()
			+ ". but element count is " + elements.size();

		/*
		System.out.printf("RESULT : %d <= MaxPipedItemCount(%d). OK!\n\n", 
						  elements.size(), arcusClient.getMaxPipedItemCount());
		*/

		return elements;
	}

	private void checkPipedInsert(int failCount, String key, List<Element<Object>> elements)
		throws InterruptedException, ExecutionException {
		
		CollectionFuture<Map<Integer, CollectionOperationStatus>> insertFuture = null;

		System.out.printf("Piped insert %d ~ %d elements.\n",
						  elements.get(0).getLongBkey(), elements.get(elements.size() - 1).getLongBkey());

		insertFuture = arcusClient.asyncBopPipedInsertBulk(key, elements, null);
		Map<Integer, CollectionOperationStatus> insertResult = insertFuture.get();

		assert insertResult.size() == failCount :
			"Expected insertion fail count is " + failCount
			+ ". but return fail count is " + insertResult.size();
		
		System.out.printf("RESULT : fail count %d. OK!\n\n",
				insertResult.size());

		if (failCount > 0) {
			/* 9 ~ 0 element don't be inserted. failed. There are trimmed */
			System.out.printf("Check insertion fail result %d elements.\n", failCount);
			for (Map.Entry<Integer, CollectionOperationStatus> entry : insertResult.entrySet()) {
				assert entry.getValue().getResponse() == CollectionResponse.OUT_OF_RANGE : 
					entry.getKey() + " bkey response must be OUT_OF_RANGE. " 
					+ "but return response is " + entry.getValue().getResponse();
				
				System.out.printf("RESULT : bkey %d | %s\n",
								  elements.get(entry.getKey()).getLongBkey(), entry.getValue());
			}
			System.out.printf("OK!\n\n");
		}
	}

	private void checkBkey(String key, long bkey, CollectionResponse... expectedResList) 
		throws InterruptedException, ExecutionException {
		CollectionFuture<Map<Long, Element<Object>>> getFuture;
		@SuppressWarnings("unused")
		Map<Long, Element<Object>> getResult;
		
		System.out.printf("Get element(bkey %d) and check reponse.\n", bkey);
		getFuture = arcusClient.asyncBopGet(key, bkey, null, false, false);
		
		getResult = getFuture.get();
		if (expectedResList.length > 1) {
			/* this is very special case */ 
			assert getFuture.getOperationStatus().getResponse() == expectedResList[0] ||
				   getFuture.getOperationStatus().getResponse() == expectedResList[1] :
				"Expected Response(bkey " + bkey + ") is "
				+ expectedResList[0] + " or " + expectedResList[1]
				+ "but Return Response is "
				+ getFuture.getOperationStatus().getMessage();
		} else {
			assert getFuture.getOperationStatus().getResponse() == expectedResList[0] :
					"Expected Response(bkey " + bkey + ") is "
					+ expectedResList[0]
					+ "but Return Response is "
					+ getFuture.getOperationStatus().getMessage();
		}
		System.out.printf("RESULT : %s. OK!\n\n", getFuture.getOperationStatus().getResponse());
	}

	private void checkAttr(String key)
		throws InterruptedException, ExecutionException {
		
		CollectionFuture<CollectionAttributes> attrFuture = null;
		attrFuture = arcusClient.asyncGetAttr(key);
		CollectionAttributes attrResult = attrFuture.get();
		
		System.out.println("ATTR type = " + attrResult.getType().getStringValue());
		System.out.println("ATTR flags = " + attrResult.getFlags());
		System.out.println("ATTR expiretime = " + attrResult.getExpireTime());
		System.out.println("ATTR count = " + attrResult.getCount());
		System.out.println("ATTR maxcount = " + attrResult.getMaxCount());
		System.out.println("ATTR overflowaction = " + attrResult.getOverflowAction());
		System.out.println("ATTR readable = " + attrResult.getReadable());
		System.out.println("ATTR maxbkeyrange = " + attrResult.getMaxBkeyRange());
		System.out.println("ATTR minbkey = " + attrResult.getMinBkey());
		System.out.println("ATTR maxbkey = " + attrResult.getMaxBkey());
		System.out.println("ATTR trimmed = " + attrResult.getTrimmed());
		
		System.out.printf("\n");
	}
}
