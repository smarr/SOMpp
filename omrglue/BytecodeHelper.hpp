/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2016, 2016
 *
 *  This program and the accompanying materials are made available
 *  under the terms of the Eclipse Public License v1.0 and
 *  Apache License v2.0 which accompanies this distribution.
 *
 *      The Eclipse Public License is available at
 *      http://www.eclipse.org/legal/epl-v10.html
 *
 *      The Apache License v2.0 is available at
 *      http://www.opensource.org/licenses/apache2.0.php
 *
 * Contributors:
 *    Multiple authors (IBM Corp.) - initial implementation and documentation
 *******************************************************************************/
 
#ifndef BYTECODEHELPER_INCL
#define BYTECODEHELPER_INCL

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <dlfcn.h>
#include <errno.h>

class BytecodeHelper {

public:
	static constexpr const char* BYTECODEHELPER_FILE = __FILE__;
	
	static int64_t getClass(int64_t object);
	static const char* GET_CLASS_LINE;
	
	static int64_t getSuperClass(int64_t object);
	static const char* GET_SUPER_CLASS_LINE;

	static int64_t getGlobal(int64_t symbol);
	static const char* GET_GLOBAL_LINE;
	
	static int64_t getNewBlock(int64_t framePtr, int64_t blockMethod, int64_t numOfArgs);
	static const char* GET_NEW_BLOCK_LINE;
	
	static int64_t newInteger(int64_t value);
	static const char* NEW_INTEGER_LINE;
	
	static int64_t newDouble(double value);
	static const char* NEW_DOUBLE_LINE;
	
	static int64_t getFieldFrom(int64_t selfPtr, int64_t fieldIndex);
	static const char* GET_FIELD_FROM_LINE;
	
	static void	setFieldTo(int64_t selfPtr, int64_t fieldIndex, int64_t valuePtr);
	static const char* SET_FIELD_TO_LINE;
	
	static int64_t getInvokable(int64_t receiverClazz, int64_t signature);
	static const char* GET_INVOKABLE_LINE;
	
	static int64_t doSendIfRequired(int64_t interp, int64_t framePtr, int64_t invokablePtr, int64_t receiverPtr, int64_t signaturePtr, int64_t bytecodeIndex);
	static const char* DO_SEND_IF_REQUIRED_LINE;
	
	static int64_t doSuperSendHelper(int64_t interp, int64_t framePtr, int64_t signaturePtr, int64_t bytecodeIndex);
	static const char* DO_SUPER_SEND_HELPER_LINE;
	
	static void popFrameAndPushResult(int64_t interp, int64_t framePtr, int64_t result);
	static const char* POP_FRAME_AND_PUSH_RESULT_LINE;
	
	static int64_t popToContext(int64_t interp, int64_t framePtr);
	static const char* POP_TO_CONTEXT_LINE;
};
	
#endif /* BYTECODEHELPER_INCL */
