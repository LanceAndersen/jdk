/*
 * Copyright (c) 2007, 2025, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include <jvmti.h>
#include <aod.hpp>
#include <jvmti_aod.hpp>

extern "C" {

/*
 * Agent receives expected number of ClassPrepare events and finishes work
 * (events should be provoked by target application)
 */

#define EXPECTED_EVENTS_NUMBER 500

static Options* options = nullptr;
static const char* agentName;

static jrawMonitorID eventsCounterMonitor;

static volatile int eventsCounter = 0;

void JNICALL classPrepareHandler(
        jvmtiEnv *jvmti,
        JNIEnv* jni,
        jthread thread,
        jclass klass) {
    int success = 1;
    bool finish = false;
    char className[MAX_STRING_LENGTH];
    jint loadedClassesCount;
    jclass *loadedClasses;

    if (!nsk_jvmti_aod_getClassName(jvmti, klass, className)) {
        nsk_jvmti_aod_disableEventAndFinish(agentName, JVMTI_EVENT_CLASS_PREPARE, 0, jvmti, jni);
        return;
    }

    if (!NSK_JVMTI_VERIFY(jvmti->GetLoadedClasses(&loadedClassesCount, &loadedClasses))) {
        NSK_COMPLAIN1("%s: failed to get loaded classes\n", agentName);
        nsk_jvmti_aod_disableEventAndFinish(agentName, JVMTI_EVENT_CLASS_PREPARE, 0, jvmti, jni);
        return;
    }

    nsk_jvmti_aod_deallocate(jvmti, (unsigned char*)loadedClasses);

    if (NSK_JVMTI_VERIFY(jvmti->RawMonitorEnter(eventsCounterMonitor))) {

        eventsCounter++;

        NSK_DISPLAY3("%s: ClassPrepare event received for class '%s' (eventsCounter: %d)\n", agentName, className, eventsCounter);

        if (eventsCounter == EXPECTED_EVENTS_NUMBER) {
            NSK_DISPLAY2("%s: all expected events were received (eventsCounter: %d)\n", agentName, eventsCounter);
            finish = true;
        }

        if (!NSK_JVMTI_VERIFY(jvmti->RawMonitorExit(eventsCounterMonitor))) {
            success = 0;
        }
    } else {
        success = 0;
    }

    if (finish || !success) {
        nsk_jvmti_aod_disableEventAndFinish(agentName, JVMTI_EVENT_CLASS_PREPARE, success, jvmti, jni);
    }
}

#ifdef STATIC_BUILD
JNIEXPORT jint JNI_OnLoad_attach045Agent01(JavaVM *jvm, char *options, void *reserved) {
    return JNI_VERSION_1_8;
}
#endif

JNIEXPORT jint JNICALL
#ifdef STATIC_BUILD
Agent_OnAttach_attach045Agent01(JavaVM *vm, char *optionsString, void *reserved)
#else
Agent_OnAttach(JavaVM *vm, char *optionsString, void *reserved)
#endif
{
    jvmtiEventCallbacks eventCallbacks;
    jvmtiEnv* jvmti;
    JNIEnv* jni;

    options = (Options*) nsk_aod_createOptions(optionsString);
    if (!NSK_VERIFY(options != nullptr))
        return JNI_ERR;

    agentName = nsk_aod_getOptionValue(options, NSK_AOD_AGENT_NAME_OPTION);

    jni = (JNIEnv*) nsk_aod_createJNIEnv(vm);
    if (jni == nullptr)
        return JNI_ERR;

    jvmti = nsk_jvmti_createJVMTIEnv(vm, reserved);
    if (!NSK_VERIFY(jvmti != nullptr))
        return JNI_ERR;

    if (!NSK_JVMTI_VERIFY(jvmti->CreateRawMonitor("attach045-agent01-eventsCounterMonitor", &eventsCounterMonitor))) {
        return JNI_ERR;
    }

    memset(&eventCallbacks,0, sizeof(eventCallbacks));
    eventCallbacks.ClassPrepare = classPrepareHandler;
    if (!NSK_JVMTI_VERIFY(jvmti->SetEventCallbacks(&eventCallbacks, sizeof(eventCallbacks)))) {
        return JNI_ERR;
    }

    if (!(nsk_jvmti_aod_enableEvent(jvmti, JVMTI_EVENT_CLASS_PREPARE))) {
        return JNI_ERR;
    }

    NSK_DISPLAY1("%s: initialization was done\n", agentName);

    if (!NSK_VERIFY(nsk_aod_agentLoaded(jni, agentName)))
        return JNI_ERR;

    return JNI_OK;
}


}
