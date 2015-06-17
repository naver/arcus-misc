import net.spy.memcached.ArcusClient;
import net.spy.memcached.ConnectionFactoryBuilder;
import net.spy.memcached.internal.OperationFuture;

public abstract class ArcusTest {
	private String ARCUS_ADMIN = "127.0.0.1:2181";
	private String SERVICE_CODE = "test";
	protected ArcusClient arcusClient;
	protected String KEY_PREFIX = "ARCUS_TEST";

	public abstract void testStart() throws Exception;

	public void beforeTest() {
		System.out.println();
		arcusClient = 
				ArcusClient.createArcusClient(ARCUS_ADMIN, SERVICE_CODE, new ConnectionFactoryBuilder());
		System.out.println();
	}

	public void afterTest() {
		System.out.println("Test Key flush.");
		OperationFuture<Boolean> flushFuture = arcusClient.flush(KEY_PREFIX);
		try {
			System.out.printf("RESULT : %s\n", flushFuture.get());
		} catch (Exception e) {
			e.printStackTrace();
		}
		System.out.println();
		arcusClient.shutdown();
		System.out.println();
	}

	public void parseArgs(String[] args) {
		for (int i = 0; i < args.length; i++) {
			if (args[i].equals("-ARCUS_ADMIN")) {
				if (++i >= args.length) {
					System.out.println("-ARCUS_ADMIN requires ip:port list");
					System.exit(0);
				}
				ARCUS_ADMIN = args[i];
			} else if (args[i].equals("-SERVICE_CODE")) {
				if (++i >= args.length) {
					System.out.println("-SERVICE_CODE requires service string");
					System.exit(0);
				}
				SERVICE_CODE = args[i];
			} else {
				System.out.println("Unknown Arguments");
				this.printUsage();
			}
		}
	}
	
	public void printUsage() {
		String usageTxt = "AsyncBopPipedInsertBulkTest [-ARCUS_ADMIN 127.0.0.1:2181] [-SERVICE_CODE \"test\"]";
		System.out.println(usageTxt);
		System.exit(0);
	}
}