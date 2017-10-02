
#include "tpch/load_db.h"

#define PROFILE(n,x) { struct timeval p0, p1; \
        gettimeofday(&p0, NULL); L e = x; CHECK(e,n); gettimeofday(&p1, NULL); \
        P("[Profiling] Line %d: %g ms\n", n,calcInterval(p0,p1)/1000.0); }

#define TEST_QUERY(n) case n: testTPCHQ##n(); break

#include "tpch/test_q1.h"
#include "tpch/test_q4.h"
#include "tpch/test_q6.h"
#include "tpch/test_q9.h"
#include "tpch/test_q12.h"
#include "tpch/test_q99.h"

void testTPCH(L x){
	switch(x){
		TEST_QUERY(1);
		TEST_QUERY(4);
		TEST_QUERY(6);
		TEST_QUERY(9);  /* testing */
		TEST_QUERY(12);
		TEST_QUERY(99); /* experiments */
		default: P("No such test for query %lld yet\n",x); break;
	}
}
