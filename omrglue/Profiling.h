/*******************************************************************************
 *
 * (c) Copyright IBM Corp. 2016
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
 
#ifndef OMRGLUE_PROFILING_H_
#define OMRGLUE_PROFILING_H_

void som_omr_checkSampleStack(struct VMFrame *frame);
void som_omr_add_class_methods_to_dictionary(struct VMClass *clazz);


#endif /* OMRGLUE_PROFILING_H_ */
