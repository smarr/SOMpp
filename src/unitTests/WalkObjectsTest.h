/*
 * WalkObjectsTest.h
 *
 *  Created on: 13.01.2011
 *      Author: christian
 */

#ifndef WALKOBJECTSTEST_H_
#define WALKOBJECTSTEST_H_

/*
 * WalkObjectsTest.cpp
 *
 *  Created on: 12.01.2011
 *      Author: christian
 */
#include <cppunit/extensions/HelperMacros.h>

using namespace std;

class WalkObjectsTest: public CPPUNIT_NS::TestCase {

	CPPUNIT_TEST_SUITE(WalkObjectsTest);
	CPPUNIT_TEST(testWalkArray);
	CPPUNIT_TEST(testWalkBlock);
	CPPUNIT_TEST(testWalkClass);
	CPPUNIT_TEST(testWalkDouble);
	CPPUNIT_TEST(testWalkEvaluationPrimitive);
	CPPUNIT_TEST(testWalkFrame);
	CPPUNIT_TEST(testWalkInteger);
	CPPUNIT_TEST(testWalkBigInteger);
	CPPUNIT_TEST(testWalkString);
	CPPUNIT_TEST(testWalkMethod);
	CPPUNIT_TEST(testWalkObject);
	CPPUNIT_TEST(testWalkPrimitive);
	CPPUNIT_TEST(testWalkSymbol);
	CPPUNIT_TEST_SUITE_END();

public:
	inline void setUp(void) {
	}
	inline void tearDown(void) {
	}

private:
	void testWalkArray();
	void testWalkBlock();
	void testWalkClass();
	void testWalkDouble();
	void testWalkEvaluationPrimitive();
	void testWalkFrame();
	void testWalkFreeObject();
	void testWalkInteger();
	void testWalkBigInteger();
	void testWalkString();
	void testWalkMethod();
	void testWalkObject();
	void testWalkPrimitive();
	void testWalkSymbol();

};

#endif /* WALKOBJECTSTEST_H_ */
