#include <robovm.h>

Class* Java_java_lang_reflect_Field_getDeclaringClass(Env* env, Class* clazz, jlong fieldPtr) {
    Field* field = (Field*) LONG_TO_PTR(fieldPtr);
    return field->clazz;
}

Object* Java_java_lang_reflect_Field_getName(Env* env, Class* clazz, jlong fieldPtr) {
    Field* field = (Field*) LONG_TO_PTR(fieldPtr);
    return rvmNewStringUTF(env, field->name, -1);
}

jint Java_java_lang_reflect_Field_getModifiers(Env* env, Class* clazz, jlong fieldPtr) {
    Field* field = (Field*) LONG_TO_PTR(fieldPtr);
    return (field->access & FIELD_ACCESS_MASK) & ~(ACC_SYNTHETIC);
}

Class* Java_java_lang_reflect_Field_getType(Env* env, Class* clazz, jlong fieldPtr) {
    Field* field = (Field*) LONG_TO_PTR(fieldPtr);
    return rvmFindClassByDescriptor(env, field->desc, field->clazz->classLoader);
}

Object* Java_java_lang_reflect_Field_getSignatureAttribute(Env* env, Class* clazz, jlong fieldPtr) {
    Field* field = (Field*) LONG_TO_PTR(fieldPtr);
    return rvmAttributeGetFieldSignature(env, field);
}

ObjectArray* Java_java_lang_reflect_Field_getDeclaredAnnotations(Env* env, Class* clazz, jlong fieldPtr) {
    Field* field = (Field*) LONG_TO_PTR(fieldPtr);
    return rvmAttributeGetFieldRuntimeVisibleAnnotations(env, field);
}

