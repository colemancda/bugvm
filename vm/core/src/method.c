#include <robovm.h>
#include <string.h>
#include <unwind.h>
#include <hythread.h>
#include "private.h"
#include "utlist.h"

#define LOG_TAG "core.method"

DynamicLib* bootNativeLibs = NULL;
DynamicLib* mainNativeLibs = NULL;

static hythread_monitor_t nativeLibsLock;
static jvalue emptyJValueArgs[1];

static inline void obtainNativeLibsLock() {
    hythread_monitor_enter(nativeLibsLock);
}

static inline void releaseNativeLibsLock() {
    hythread_monitor_exit(nativeLibsLock);
}

static Method* findMethod(Env* env, Class* clazz, const char* name, const char* desc) {
    Method* method = rvmGetMethods(env, clazz);
    if (rvmExceptionCheck(env)) return NULL;
    for (; method != NULL; method = method->next) {
        if (!strcmp(method->name, name) && !strcmp(method->desc, desc)) {
            return method;
        }
    }
    return NULL;
}

static Method* getMethod(Env* env, Class* clazz, const char* name, const char* desc) {
    if (!strcmp("<init>", name) || !strcmp("<clinit>", name)) {
        // Constructors and static initializers are not inherited so we shouldn't check with the superclasses.
        return findMethod(env, clazz, name, desc);
    }

    Class* c = clazz;
    for (c = clazz; c != NULL; c = c->superclass) {
        Method* method = findMethod(env, c, name, desc);
        if (rvmExceptionCheck(env)) return NULL;
        if (method) return method;
    }

    /*
     * Check with interfaces.
     * TODO: Should we really do this? Does the JNI GetMethodID() function do this?
     */
    for (c = clazz; c != NULL; c = c->superclass) {
        Interface* interface = rvmGetInterfaces(env, c);
        if (rvmExceptionCheck(env)) return NULL;
        for (; interface != NULL; interface = interface->next) {
            Method* method = getMethod(env, interface->interface, name, desc);
            if (rvmExceptionCheck(env)) return NULL;
            if (method) return method;
        }
    }

    if (CLASS_IS_INTERFACE(clazz)) {
        /*
         * Class is an interface so check with java.lang.Object.
         * TODO: Should we really do this? Does the JNI GetMethodID() function do this?
         */
        return getMethod(env, java_lang_Object, name, desc);
    }

    return NULL;
}

jboolean rvmInitMethods(Env* env) {
    if (hythread_monitor_init_with_name(&nativeLibsLock, 0, NULL) < 0) {
        return FALSE;
    }
    return TRUE;
}

jboolean rvmHasMethod(Env* env, Class* clazz, const char* name, const char* desc) {
    Method* method = getMethod(env, clazz, name, desc);
    if (rvmExceptionCheck(env)) return FALSE;
    return method ? TRUE : FALSE;
}

Method* rvmGetMethod(Env* env, Class* clazz, const char* name, const char* desc) {
    Method* method = getMethod(env, clazz, name, desc);
    if (rvmExceptionCheck(env)) return NULL;
    if (!method) {
        rvmThrowNoSuchMethodError(env, name);
        return NULL;
    }
    return method;
}

Method* rvmGetClassMethod(Env* env, Class* clazz, const char* name, const char* desc) {
    Method* method = rvmGetMethod(env, clazz, name, desc);
    if (!method) return NULL;
    if (!METHOD_IS_STATIC(method)) {
        // TODO: JNI spec doesn't say anything about throwing this
        rvmThrowIncompatibleClassChangeErrorMethod(env, clazz, name, desc);
        return NULL;
    }
    return method;
}

Method* rvmGetClassInitializer(Env* env, Class* clazz) {
    return getMethod(env, clazz, "<clinit>", "()V");
}

Method* rvmGetInstanceMethod(Env* env, Class* clazz, const char* name, const char* desc) {
    Method* method = rvmGetMethod(env, clazz, name, desc);
    if (!method) return NULL;
    if (METHOD_IS_STATIC(method)) {
        // TODO: JNI spec doesn't say anything about throwing this
        rvmThrowIncompatibleClassChangeErrorMethod(env, clazz, name, desc);
        return NULL;
    }
    return method;
}

Method* rvmFindMethodAtAddress(Env* env, void* address) {
    Class* clazz = env->vm->options->findClassAt(env, address);
    if (!clazz) return NULL;
    Method* method = rvmGetMethods(env, clazz);
    if (rvmExceptionCheck(env)) return NULL;
    for (; method != NULL; method = method->next) {
        void* start = method->impl;
        void* end = start + method->size;
        if (start && address >= start && address < end) {
            return method;
        }
    }
    // TODO: We should never end up here
    return NULL;
}

static jboolean getCallingMethodIterator(Env* env, void* pc, ProxyMethod* proxyMethod, void* data) {
    Method** result = data;

    Method* method = rvmFindMethodAtAddress(env, pc);
    if (method) {
        *result = method;
        return FALSE; // Stop iterating
    }
    return TRUE;
}

static jboolean getCallStackIterator(Env* env, void* pc, ProxyMethod* proxyMethod, void* data) {
    CallStackEntry** head = (CallStackEntry**) data;
    Method* method = NULL;
    if (proxyMethod) {
        method = (Method*) proxyMethod;
    } else {
        method = rvmFindMethodAtAddress(env, pc);
    }
    if (method) {
        CallStackEntry* entry = rvmAllocateMemory(env, sizeof(CallStackEntry));
        if (!entry) {
            return FALSE; // Stop iterating
        }
        entry->method = method;
        entry->offset = proxyMethod ? 0 : pc - method->impl;
        DL_APPEND(*head, entry);
    }
    return TRUE;
}

Method* rvmGetCallingMethod(Env* env) {
    Method* result = NULL;
    unwindIterateCallStack(env, getCallingMethodIterator, &result);
    return result;
}

CallStackEntry* rvmGetCallStack(Env* env) {
    CallStackEntry* data = NULL;
    unwindIterateCallStack(env, getCallStackIterator, &data);
    if (rvmExceptionOccurred(env)) return NULL;
    return data;
}

const char* rvmGetReturnType(const char* desc) {
    while (*desc != ')') desc++;
    desc++;
    return desc;
}

const char* rvmGetNextParameterType(const char** desc) {
    const char* s = *desc;
    (*desc)++;
    switch (s[0]) {
    case 'B':
    case 'Z':
    case 'S':
    case 'C':
    case 'I':
    case 'J':
    case 'F':
    case 'D':
        return s;
    case '[':
        rvmGetNextParameterType(desc);
        return s;
    case 'L':
        while (**desc != ';') (*desc)++;
        (*desc)++;
        return s;
    case '(':
        return rvmGetNextParameterType(desc);
    }
    return 0;
}

jint rvmGetParameterCount(Method* method) {
    const char* desc = method->desc;
    jint count = 0;
    while (rvmGetNextParameterType(&desc)) {
        count++;
    }
    return count;
}

static inline jboolean isFpType(char type) {
    return type == 'F' || type == 'D';
}

CallInfo* initCallInfo(Env* env, Object* obj, Method* method, jboolean virtual, jvalue* args) {
    if (virtual && !(method->access & ACC_PRIVATE)) {
        // Lookup the real method to be invoked
        method = rvmGetMethod(env, obj->clazz, method->name, method->desc);
        if (!method) return NULL;
    }

    jint ptrArgsCount = 0, intArgsCount = 0, longArgsCount = 0, floatArgsCount = 0, doubleArgsCount = 0;

    ptrArgsCount = 1; // First arg is always the Env*
    if (!(method->access & ACC_STATIC)) {
        // Non-static methods takes the receiver object (this) as arg 2
        ptrArgsCount++;
    }    

    const char* desc = method->desc;
    const char* c;
    while ((c = rvmGetNextParameterType(&desc))) {
        switch (c[0]) {
        case 'Z':
        case 'B':
        case 'S':
        case 'C':
        case 'I':
            intArgsCount++;
            break;
        case 'J':
            longArgsCount++;
            break;
        case 'F':
            floatArgsCount++;
            break;
        case 'D':
            doubleArgsCount++;
            break;
        case 'L':
        case '[':
            ptrArgsCount++;
            break;
        }
    }

    void* function = method->synchronizedImpl ? method->synchronizedImpl : method->impl;

    CallInfo* callInfo = call0AllocateCallInfo(env, function, ptrArgsCount, intArgsCount, longArgsCount, floatArgsCount, doubleArgsCount);
    if (!callInfo) return NULL;

    call0AddPtr(callInfo, env);
    if (!(method->access & ACC_STATIC)) {
        call0AddPtr(callInfo, obj);
    }    

    desc = method->desc;
    jint i = 0;
    while ((c = rvmGetNextParameterType(&desc))) {
        switch (c[0]) {
        case 'Z':
            call0AddInt(callInfo, (jint) args[i++].z);
            break;
        case 'B':
            call0AddInt(callInfo, (jint) args[i++].b);
            break;
        case 'S':
            call0AddInt(callInfo, (jint) args[i++].s);
            break;
        case 'C':
            call0AddInt(callInfo, (jint) args[i++].c);
            break;
        case 'I':
            call0AddInt(callInfo, args[i++].i);
            break;
        case 'J':
            call0AddLong(callInfo, args[i++].j);
            break;
        case 'F':
            call0AddFloat(callInfo, args[i++].f);
            break;
        case 'D':
            call0AddDouble(callInfo, args[i++].d);
            break;
        case 'L':
        case '[':
            call0AddPtr(callInfo, args[i++].l);
            break;
        }
    }

    return callInfo;
}

static jvalue* va_list2jargs(Env* env, Method* method, va_list args) {
    jint argsCount = rvmGetParameterCount(method);

    if (argsCount == 0) {
        return emptyJValueArgs;
    }

    jvalue *jvalueArgs = (jvalue*) rvmAllocateMemory(env, sizeof(jvalue) * argsCount);
    if (!jvalueArgs) return NULL;

    const char* desc = method->desc;
    const char* c;
    jint i = 0;
    while ((c = rvmGetNextParameterType(&desc))) {
        switch (c[0]) {
        case 'B':
            jvalueArgs[i++].b = (jbyte) va_arg(args, jint);
            break;
        case 'Z':
            jvalueArgs[i++].z = (jboolean) va_arg(args, jint);
            break;
        case 'S':
            jvalueArgs[i++].s = (jshort) va_arg(args, jint);
            break;
        case 'C':
            jvalueArgs[i++].c = (jchar) va_arg(args, jint);
            break;
        case 'I':
            jvalueArgs[i++].i = va_arg(args, jint);
            break;
        case 'J':
            jvalueArgs[i++].j = va_arg(args, jlong);
            break;
        case 'F':
            jvalueArgs[i++].f = (jfloat) va_arg(args, jdouble);
            break;
        case 'D':
            jvalueArgs[i++].d = va_arg(args, jdouble);
            break;
        case '[':
        case 'L':
            jvalueArgs[i++].l = va_arg(args, jobject);
            break;
        }
    }

    return jvalueArgs;
}

void rvmCallVoidInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return;
    void (*f)(CallInfo*) = _call0;
    rvmPushGatewayFrame(env);
    f(callInfo);
    rvmPopGatewayFrame(env);
}

void rvmCallVoidInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return;
    rvmCallVoidInstanceMethodA(env, obj, method, jargs);
}

void rvmCallVoidInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    rvmCallVoidInstanceMethodV(env, obj, method, args);
}

Object* rvmCallObjectInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return NULL;
    Object* (*f)(CallInfo*) = (Object* (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    Object* result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

Object* rvmCallObjectInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return NULL;
    return rvmCallObjectInstanceMethodA(env, obj, method, jargs);
}

Object* rvmCallObjectInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallObjectInstanceMethodV(env, obj, method, args);
}

jboolean rvmCallBooleanInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return FALSE;
    jboolean (*f)(CallInfo*) = (jboolean (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jboolean result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jboolean rvmCallBooleanInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return FALSE;
    return rvmCallBooleanInstanceMethodA(env, obj, method, jargs);
}

jboolean rvmCallBooleanInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallBooleanInstanceMethodV(env, obj, method, args);
}

jbyte rvmCallByteInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0;
    jbyte (*f)(CallInfo*) = (jbyte (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jbyte result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jbyte rvmCallByteInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallByteInstanceMethodA(env, obj, method, jargs);
}

jbyte rvmCallByteInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallByteInstanceMethodV(env, obj, method, args);
}

jchar rvmCallCharInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0;
    jchar (*f)(CallInfo*) = (jchar (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jchar result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jchar rvmCallCharInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallCharInstanceMethodA(env, obj, method, jargs);
}

jchar rvmCallCharInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallCharInstanceMethodV(env, obj, method, args);
}

jshort rvmCallShortInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0;
    jshort (*f)(CallInfo*) = (jshort (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jshort result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jshort rvmCallShortInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallShortInstanceMethodA(env, obj, method, jargs);
}

jshort rvmCallShortInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallShortInstanceMethodV(env, obj, method, args);
}

jint rvmCallIntInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0;
    jint (*f)(CallInfo*) = (jint (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jint result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jint rvmCallIntInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallIntInstanceMethodA(env, obj, method, jargs);
}

jint rvmCallIntInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallIntInstanceMethodV(env, obj, method, args);
}

jlong rvmCallLongInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0;
    jlong (*f)(CallInfo*) = (jlong (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jlong result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jlong rvmCallLongInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallLongInstanceMethodA(env, obj, method, jargs);
}

jlong rvmCallLongInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallLongInstanceMethodV(env, obj, method, args);
}

jfloat rvmCallFloatInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0.0f;
    jfloat (*f)(CallInfo*) = (jfloat (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jfloat result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jfloat rvmCallFloatInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0.0f;
    return rvmCallFloatInstanceMethodA(env, obj, method, jargs);
}

jfloat rvmCallFloatInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallFloatInstanceMethodV(env, obj, method, args);
}

jdouble rvmCallDoubleInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, TRUE, args);
    if (!callInfo) return 0.0;
    jdouble (*f)(CallInfo*) = (jdouble (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jdouble result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jdouble rvmCallDoubleInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0.0;
    return rvmCallDoubleInstanceMethodA(env, obj, method, jargs);
}

jdouble rvmCallDoubleInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallDoubleInstanceMethodV(env, obj, method, args);
}

void rvmCallNonvirtualVoidInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return;
    void (*f)(CallInfo*) = _call0;
    rvmPushGatewayFrame(env);
    f(callInfo);
    rvmPopGatewayFrame(env);
}

void rvmCallNonvirtualVoidInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return;
    rvmCallNonvirtualVoidInstanceMethodA(env, obj, method, jargs);
}

void rvmCallNonvirtualVoidInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    rvmCallNonvirtualVoidInstanceMethodV(env, obj, method, args);
}

Object* rvmCallNonvirtualObjectInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return NULL;
    Object* (*f)(CallInfo*) = (Object* (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    Object* result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

Object* rvmCallNonvirtualObjectInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return NULL;
    return rvmCallNonvirtualObjectInstanceMethodA(env, obj, method, jargs);
}

Object* rvmCallNonvirtualObjectInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualObjectInstanceMethodV(env, obj, method, args);
}

jboolean rvmCallNonvirtualBooleanInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return FALSE;
    jboolean (*f)(CallInfo*) = (jboolean (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jboolean result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jboolean rvmCallNonvirtualBooleanInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return FALSE;
    return rvmCallNonvirtualBooleanInstanceMethodA(env, obj, method, jargs);
}

jboolean rvmCallNonvirtualBooleanInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualBooleanInstanceMethodV(env, obj, method, args);
}

jbyte rvmCallNonvirtualByteInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0;
    jbyte (*f)(CallInfo*) = (jbyte (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jbyte result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jbyte rvmCallNonvirtualByteInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallNonvirtualByteInstanceMethodA(env, obj, method, jargs);
}

jbyte rvmCallNonvirtualByteInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualByteInstanceMethodV(env, obj, method, args);
}

jchar rvmCallNonvirtualCharInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0;
    jchar (*f)(CallInfo*) = (jchar (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jchar result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jchar rvmCallNonvirtualCharInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallNonvirtualCharInstanceMethodA(env, obj, method, jargs);
}

jchar rvmCallNonvirtualCharInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualCharInstanceMethodV(env, obj, method, args);
}

jshort rvmCallNonvirtualShortInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0;
    jshort (*f)(CallInfo*) = (jshort (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jshort result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jshort rvmCallNonvirtualShortInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallNonvirtualShortInstanceMethodA(env, obj, method, jargs);
}

jshort rvmCallNonvirtualShortInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualShortInstanceMethodV(env, obj, method, args);
}

jint rvmCallNonvirtualIntInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0;
    jint (*f)(CallInfo*) = (jint (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jint result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jint rvmCallNonvirtualIntInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallNonvirtualIntInstanceMethodA(env, obj, method, jargs);
}

jint rvmCallNonvirtualIntInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualIntInstanceMethodV(env, obj, method, args);
}

jlong rvmCallNonvirtualLongInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0;
    jlong (*f)(CallInfo*) = (jlong (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jlong result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jlong rvmCallNonvirtualLongInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallNonvirtualLongInstanceMethodA(env, obj, method, jargs);
}

jlong rvmCallNonvirtualLongInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualLongInstanceMethodV(env, obj, method, args);
}

jfloat rvmCallNonvirtualFloatInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0.0f;
    jfloat (*f)(CallInfo*) = (jfloat (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jfloat result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jfloat rvmCallNonvirtualFloatInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0.0f;
    return rvmCallNonvirtualFloatInstanceMethodA(env, obj, method, jargs);
}

jfloat rvmCallNonvirtualFloatInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualFloatInstanceMethodV(env, obj, method, args);
}

jdouble rvmCallNonvirtualDoubleInstanceMethodA(Env* env, Object* obj, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, obj, method, FALSE, args);
    if (!callInfo) return 0.0;
    jdouble (*f)(CallInfo*) = (jdouble (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jdouble result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jdouble rvmCallNonvirtualDoubleInstanceMethodV(Env* env, Object* obj, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0.0;
    return rvmCallNonvirtualDoubleInstanceMethodA(env, obj, method, jargs);
}

jdouble rvmCallNonvirtualDoubleInstanceMethod(Env* env, Object* obj, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallNonvirtualDoubleInstanceMethodV(env, obj, method, args);
}

void rvmCallVoidClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return;
    void (*f)(CallInfo*) = _call0;
    rvmPushGatewayFrame(env);
    f(callInfo);
    rvmPopGatewayFrame(env);
}

void rvmCallVoidClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return;
    rvmCallVoidClassMethodA(env, clazz, method, jargs);
}

void rvmCallVoidClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    rvmCallVoidClassMethodV(env, clazz, method, args);
}

Object* rvmCallObjectClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return NULL;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return NULL;
    Object* (*f)(CallInfo*) = (Object* (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    Object* result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

Object* rvmCallObjectClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return NULL;
    return rvmCallObjectClassMethodA(env, clazz, method, jargs);
}

Object* rvmCallObjectClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallObjectClassMethodV(env, clazz, method, args);
}

jboolean rvmCallBooleanClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return FALSE;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return FALSE;
    jboolean (*f)(CallInfo*) = (jboolean (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jboolean result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jboolean rvmCallBooleanClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return FALSE;
    return rvmCallBooleanClassMethodA(env, clazz, method, jargs);
}

jboolean rvmCallBooleanClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallBooleanClassMethodV(env, clazz, method, args);
}

jbyte rvmCallByteClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0;
    jbyte (*f)(CallInfo*) = (jbyte (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jbyte result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jbyte rvmCallByteClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallByteClassMethodA(env, clazz, method, jargs);
}

jbyte rvmCallByteClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallByteClassMethodV(env, clazz, method, args);
}

jchar rvmCallCharClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0;
    jchar (*f)(CallInfo*) = (jchar (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jchar result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jchar rvmCallCharClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallCharClassMethodA(env, clazz, method, jargs);
}

jchar rvmCallCharClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallCharClassMethodV(env, clazz, method, args);
}

jshort rvmCallShortClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0;
    jshort (*f)(CallInfo*) = (jshort (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jshort result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jshort rvmCallShortClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallShortClassMethodA(env, clazz, method, jargs);
}

jshort rvmCallShortClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallShortClassMethodV(env, clazz, method, args);
}

jint rvmCallIntClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0;
    jint (*f)(CallInfo*) = (jint (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jint result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jint rvmCallIntClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallIntClassMethodA(env, clazz, method, jargs);
}

jint rvmCallIntClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallIntClassMethodV(env, clazz, method, args);
}

jlong rvmCallLongClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0;
    jlong (*f)(CallInfo*) = (jlong (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jlong result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jlong rvmCallLongClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0;
    return rvmCallLongClassMethodA(env, clazz, method, jargs);
}

jlong rvmCallLongClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallLongClassMethodV(env, clazz, method, args);
}

jfloat rvmCallFloatClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0.0f;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0.0f;
    jfloat (*f)(CallInfo*) = (jfloat (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jfloat result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jfloat rvmCallFloatClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0.0f;
    return rvmCallFloatClassMethodA(env, clazz, method, jargs);
}

jfloat rvmCallFloatClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallFloatClassMethodV(env, clazz, method, args);
}

jdouble rvmCallDoubleClassMethodA(Env* env, Class* clazz, Method* method, jvalue* args) {
    CallInfo* callInfo = initCallInfo(env, NULL, method, FALSE, args);
    if (!callInfo) return 0.0;
    rvmInitialize(env, method->clazz);
    if (rvmExceptionOccurred(env)) return 0.0;
    jdouble (*f)(CallInfo*) = (jdouble (*)(CallInfo*)) _call0;
    rvmPushGatewayFrame(env);
    jdouble result = f(callInfo);
    rvmPopGatewayFrame(env);
    return result;
}

jdouble rvmCallDoubleClassMethodV(Env* env, Class* clazz, Method* method, va_list args) {
    jvalue* jargs = va_list2jargs(env, method, args);
    if (!jargs) return 0.0;
    return rvmCallDoubleClassMethodA(env, clazz, method, jargs);
}

jdouble rvmCallDoubleClassMethod(Env* env, Class* clazz, Method* method, ...) {
    va_list args;
    va_start(args, method);
    return rvmCallDoubleClassMethodV(env, clazz, method, args);
}

jboolean rvmRegisterNative(Env* env, NativeMethod* method, void* impl) {
    method->nativeImpl = impl;
    return TRUE;
}

jboolean rvmUnregisterNative(Env* env, NativeMethod* method) {
    method->nativeImpl = NULL;
    return TRUE;
}

void* rvmResolveNativeMethodImpl(Env* env, NativeMethod* method, const char* shortMangledName, const char* longMangledName, ClassLoader* classLoader, void** ptr) {
    void* f = method->nativeImpl;
    if (!f) {
        DynamicLib* nativeLibs = NULL;
        if (!classLoader || classLoader->parent == NULL) {
            // This is the bootstrap classloader
            nativeLibs = bootNativeLibs;
        } else if (classLoader->parent->parent == NULL && classLoader->object.clazz->classLoader == NULL) {
            // This is the system classloader
            nativeLibs = mainNativeLibs;
        } else {
            // Unknown classloader
            rvmThrowUnsatisfiedLinkError(env);
            return NULL;
        }

        obtainNativeLibsLock();

        TRACEF("Searching for native method using short name: %s", shortMangledName);
        f = rvmFindDynamicLibSymbol(env, nativeLibs, shortMangledName, TRUE);
        if (f) {
            TRACEF("Found native method using short name: %s", shortMangledName);
        } else if (!strcmp(shortMangledName, longMangledName)) {
            TRACEF("Searching for native method using long name: %s", longMangledName);
            void* f = rvmFindDynamicLibSymbol(env, nativeLibs, longMangledName, TRUE);
            if (f) {
                TRACEF("Found native method using long name: %s", longMangledName);
            }
        }

        method->nativeImpl = f;

        releaseNativeLibsLock();
    }

    if (!f) {
        rvmThrowUnsatisfiedLinkError(env);
        return NULL;
    }
    // TODO: Remember ptr to allow it to be reset when the JNI RegisterNatives/UnregisterNatives functions are called
    *ptr = f;
    return f;
}


jboolean rvmLoadNativeLibrary(Env* env, const char* path, ClassLoader* classLoader) {
    DynamicLib** nativeLibs = NULL;
    if (!classLoader || classLoader->parent == NULL) {
        // This is the bootstrap classloader
        nativeLibs = &bootNativeLibs;
    } else if (classLoader->parent->parent == NULL && classLoader->object.clazz->classLoader == NULL) {
        // This is the system classloader
        nativeLibs = &mainNativeLibs;
    } else {
        // Unknown classloader
        if (bootNativeLibs) {
            // if bootNativeLibs is NULL we're being called from rvmStartup() and we cannot throw exceptions.
            rvmThrowUnsatisfiedLinkError(env);
        }
        return FALSE;
    }

    DynamicLib* lib = rvmOpenDynamicLib(env, path);
    if (!lib) {
        if (!rvmExceptionOccurred(env)) {
            if (bootNativeLibs) {
                // if bootNativeLibs is NULL we're being called from rvmStartup() and we cannot throw exceptions.
                rvmThrowUnsatisfiedLinkError(env);
            }
        }
        return FALSE;
    }

    obtainNativeLibsLock();

    if (rvmHasDynamicLib(env, lib, *nativeLibs)) {
        // The lib is already in nativeLibs
        rvmCloseDynamicLib(env, lib);
        releaseNativeLibsLock();
        return TRUE;
    }

    jint (*JNI_OnLoad)(JavaVM*, void*) = rvmFindDynamicLibSymbol(env, lib, "JNI_OnLoad", FALSE);
    if (JNI_OnLoad) {
        // TODO: Check that JNI_OnLoad returns a supported JNI version?
        JNI_OnLoad(&env->vm->javaVM, NULL);
        if (rvmExceptionOccurred(env)) {
            releaseNativeLibsLock();
            return FALSE;
        }
    }

    rvmAddDynamicLib(env, lib, nativeLibs);

    releaseNativeLibsLock();

    return TRUE;
}

