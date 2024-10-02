// vd_meta.h - A C reflection library
// 
// -------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright (c) 2024 Michael Dodis
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -------------------------------------------------------------------------------------------------
//
// EXAMPLES
//
// Register a struct
// struct
// 
//
#if defined(__INTELLISENSE__) || defined(__CLANGD__)
#define VD_META_IMPLEMENTATION
#endif

#ifndef VD_META_H
#define VD_META_H

#include <stdint.h>

#ifndef VD_META_OPTION_DEFINE_DEFAULT_TYPES
#define VD_META_OPTION_DEFINE_DEFAULT_TYPES 1
#endif

#define VD_META_ALLOC_PROC(name) void *name(void *ptr, size_t prevsize, size_t newsize, void *c)
typedef VD_META_ALLOC_PROC(VD_Meta_AllocProc);

typedef struct VD_Meta_Descriptor VD_Meta_Descriptor;

typedef enum {
    VD_META_TYPE_PRIMITIVE,
    VD_META_TYPE_STRING,
    VD_META_TYPE_ARRAY,
    VD_META_TYPE_OBJECT,
} VD_Meta_TypeClass;

typedef enum {
    VD_META_PRIMITIVE_TYPE_I8,
    VD_META_PRIMITIVE_TYPE_U8,
    VD_META_PRIMITIVE_TYPE_I16,
    VD_META_PRIMITIVE_TYPE_U16,
    VD_META_PRIMITIVE_TYPE_I32,
    VD_META_PRIMITIVE_TYPE_U32,
    VD_META_PRIMITIVE_TYPE_I64,
    VD_META_PRIMITIVE_TYPE_U64,
    VD_META_PRIMITIVE_TYPE_F32,
    VD_META_PRIMITIVE_TYPE_F64,
    VD_META_PRIMITIVE_TYPE_CHAR8,
} VD_Meta_PrimitiveType;

typedef enum {
    /** A null terminated utf-8 string. */
    VD_META_STRING_TYPE_CSTRING,

    /** A utf-8 string with a len property */
    VD_META_STRING_TYPE_SLICE,
} VD_Meta_StringType;

enum {
    VD_META_SUCCESS = 0,
    VD_META_DESCRIPTOR_NOT_FOUND = -1,
    VD_META_INVALID_JSON = -2,
    VD_META_INVALID_TYPE = -3,
    VD_META_TYPE_NOT_FOUND = -4,
    VD_META_TYPE_MISMATCH = -5,
};

typedef struct {
    uint64_t value;
} VD_Meta_ID;

typedef enum {
    VD_META_FIELD_FLAG_NONE = 0,
    VD_META_FIELD_FLAG_FIXED_ARRAY = 1 << 0,
    VD_META_FIELD_FLAG_INTRUSIVE_ARRAY = 1 << 1,
} VD_Meta_FieldFlags;

typedef struct {
    const char          *name;
    VD_Meta_ID          type;
    size_t              offset;
    VD_Meta_FieldFlags  flags;

    union {
        size_t      len;
    } fixed_array;
} VD_Meta_Field;

typedef struct {
    VD_Meta_ID  id;
    const char  *name;
} VD_Meta__IDEntry;

struct VD_Meta_Descriptor {
    /** The name (id) of this element */
    const char              *name;
    /** The general class of this element. */
    VD_Meta_TypeClass       type_class;
    /** The size (in bytes) of this element */
    size_t                  size;
    /** The ID of the type, leave 0 to create new one. */
    VD_Meta_ID              id;

    struct {
        VD_Meta_PrimitiveType type;
    } primitive;

    struct {
        VD_Meta_Field       *fields;
        size_t              len;
    } object;

    struct {
        VD_Meta_ID          sub_id;
        size_t              len;
    } array;

    struct {
        VD_Meta_StringType  type;
        VD_Meta_ID          len_id;
        size_t              len_offset;
    } string;
};

#define VD_META_INTRUSIVE_ARRAY_ADD_PROC(name) \
    void name(void **array, void *element, size_t size, void *c)
#define VD_META_INTRUSIVE_ARRAY_LEN_PROC(name) size_t name(void *array, void *c)

typedef VD_META_INTRUSIVE_ARRAY_ADD_PROC(VD_Meta_IntrusiveArrayAddProc);
typedef VD_META_INTRUSIVE_ARRAY_LEN_PROC(VD_Meta_IntrusiveArrayLenProc);

typedef struct {
    VD_Meta_IntrusiveArrayAddProc   *add;
    VD_Meta_IntrusiveArrayLenProc   *len;
    void                            *c;
} VD_Meta_IntrusiveArrayProperties;

typedef struct {

    VD_Meta_AllocProc                   *alloc;
    void                                *alloc_ctx;
    VD_Meta_IntrusiveArrayProperties    intrusive_array_props;

    /** Hashes ID -> Descriptor */
    VD_Meta_Descriptor  **table;
    size_t              table_cap;

    /** Hashes Name -> ID */
    VD_Meta__IDEntry    **type_ids;
    size_t              type_ids_cap;

    uint64_t            next_id;
} VD_Meta_Registry;

typedef struct {
    /** 
     * Writes the type name of @see VD_META_TYPE_OBJECT so that parsers can validate that they are
     * reading the correct type.
     */
    int                 include_validation;
    /** The allocator if null, will use the default one. */
    VD_Meta_AllocProc   *alloc;
    /** Extra parameter to pass to the allocator. */
    void                *alloc_ctx;
    /** Set to -1 to output the minifed version, set to 0 to output a pretty (spaced) version. */
    int                 pretty;
} VD_Meta_WriteOptions;

typedef struct {
    VD_Meta_AllocProc   *alloc;
    void                *alloc_ctx;

    VD_Meta_AllocProc   *temp_alloc;
    void                *temp_alloc_ctx;

    VD_Meta_ID          type;
    VD_Meta_ID          *out_type;
} VD_Meta_ParseOptions;

/**
 * @brief Initialize the meta registry
 * @param registry The registry to initialize
 * @note Assign the VD_Meta_Registry.alloc member to NULL to use the default allocator
 */
int vd_meta_init(VD_Meta_Registry *registry);

/**
 * @brief Register a type
 * @param registry      The registry to use
 * @param descriptor    The descriptor of the type
 * @return The ID of the type
 * @note The only reserved name in the descriptor is "--vd-meta-type-name--"
 */
VD_Meta_ID vd_meta_register(VD_Meta_Registry *registry, VD_Meta_Descriptor *descriptor);

VD_Meta_ID vd_meta_get_id(VD_Meta_Registry *registry, const char *name);
VD_Meta_Descriptor *vd_meta_get_descriptor(VD_Meta_Registry *registry, VD_Meta_ID id);

/**
 * @brief Parse a json string into an object
 * @param registry  The registry to use
 * @param options   The parse options
 * @param json      The json string
 * @param len       The length of the json string
 * @param out_obj   The output object
 * @return          0 on success, < 0 on error
 */
int vd_meta_parse_json(
    VD_Meta_Registry *registry,
    VD_Meta_ParseOptions *options,
    const char *json,
    size_t len,
    void **out_obj);

/**
 * @brief Write an object to a json string
 * @param registry  The registry to use
 * @param object    The object to write
 * @param type      The type of the object
 * @param alloc     The allocator to use, will use the registry's allocator if NULL
 * @param alloc_ctx The allocator context
 * @param pretty    If the output should be pretty
 * @param out_json  The output json string
 * @param out_len   The length of the output json string
 */
int vd_meta_write_json(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_ID type,
    VD_Meta_WriteOptions *options,
    char **out_json,
    size_t *out_len);

/**
 * @brief Deinitialize the meta registry
 * @param registry The registry to deinitialize
 * @note If you free the memory already you don't need to call this function
 */
int vd_meta_deinit(VD_Meta_Registry *registry);

#define VD_META_ID_1(type) VD_META_ID_##type
#define VD_META_ID(type) VD_META_ID_1(type)

#define VD_META_DECL_TYPE(type) VD_Meta_ID VD_META_ID(type)
#define VD_META_DEFN_TYPE(registry, type, ...) \
    VD_META_ID(type) = vd_meta_register(registry, & (VD_Meta_Descriptor) __VA_ARGS__)

#if VD_META_OPTION_DEFINE_DEFAULT_TYPES
extern VD_META_DECL_TYPE(int8_t);
extern VD_META_DECL_TYPE(uint8_t);
extern VD_META_DECL_TYPE(int16_t);
extern VD_META_DECL_TYPE(uint16_t);
extern VD_META_DECL_TYPE(int32_t);
extern VD_META_DECL_TYPE(uint32_t);
extern VD_META_DECL_TYPE(int64_t);
extern VD_META_DECL_TYPE(uint64_t);
extern VD_META_DECL_TYPE(float);
extern VD_META_DECL_TYPE(double);
extern VD_META_DECL_TYPE(char);
extern VD_META_DECL_TYPE(CString);
#endif

#ifdef VD_META_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

static inline VD_META_ALLOC_PROC(vd_meta__default_alloc_proc)
{
    if (newsize == 0) {
        free(ptr);
        return 0;
    }

    return realloc(ptr, newsize);
}

typedef struct {
    uint32_t len;
    uint32_t cap;
} VD_Meta__ArrayHeader;

#define VD_META_ARRAY_HEADER(a) \
    ((VD_Meta__ArrayHeader*)((char*)(a) - sizeof(VD_Meta__ArrayHeader)))
#define VD_META_ARRAY_LEN(a)                    (a ? (VD_META_ARRAY_HEADER(a))->len : 0)
#define VD_META_ARRAY_CAP(a)                    (a ? (VD_META_ARRAY_HEADER(a))->cap : 0)
#define VD_META_ARRAY_ADD(a, x, c, cx) \
    (VD_META_ARRAY_CHECK_GROW(a, 1, c, cx), (a)[VD_META_ARRAY_HEADER(a)->len++] = x)

#define VD_META_ARRAY_ADDN(a, n, c, cx) \
    (VD_META_ARRAY_CHECK_GROW(a, n, c, cx), VD_META_ARRAY_HEADER(a)->len += n)

#define VD_META_ARRAY_GROW(a, b, c, d, dx) \
    ((a) = vd_meta__array_grow(a, sizeof(*(a)), (b), (c), d, dx))
#define VD_META_ARRAY_CHECK_GROW(a,n, c, cx) \
    ((!(a) || VD_META_ARRAY_LEN(a) + (n) > VD_META_ARRAY_HEADER(a)->cap) \
    ? (VD_META_ARRAY_GROW(a, n, 0, c, cx), 0) : 0)

#define VD_META_ARRAY_FREE(a, al, alc) \
    (al(a ? VD_META_ARRAY_HEADER(a) : 0, 0, 0, alc), a = 0)

static inline void *vd_meta__array_grow(
    void *a,
    uint32_t tsize,
    uint32_t addlen,
    uint32_t mincap,
    VD_Meta_AllocProc *alloc,
    void *c)
{
    uint32_t min_len = VD_META_ARRAY_LEN(a) + addlen;

    if (min_len > mincap) {
        mincap = min_len;
    }

    if (mincap <= VD_META_ARRAY_CAP(a)) {
        return a;
    }

    if (mincap < (2 * VD_META_ARRAY_CAP(a))) {
        mincap = 2 * VD_META_ARRAY_CAP(a);
    } else if (mincap < 4) {
        mincap = 4;
    }

    void *b = alloc(
        a ? VD_META_ARRAY_HEADER(a) : 0,
        VD_META_ARRAY_CAP(a) == 0 ? 0 : tsize * VD_META_ARRAY_CAP(a) + sizeof(VD_Meta__ArrayHeader),
        tsize * mincap + sizeof(VD_Meta__ArrayHeader),
        c);

    b = (char*)b + sizeof(VD_Meta__ArrayHeader);
    if (a == 0) {
        VD_META_ARRAY_HEADER(b)->len = 0;
    }

    VD_META_ARRAY_HEADER(b)->cap = mincap;
    return b;
}

typedef struct VD_Meta__ArenaBlock VD_Meta__ArenaBlock;
struct VD_Meta__ArenaBlock 
{
    size_t len;
    size_t cap;
    VD_Meta__ArenaBlock *next;
    VD_Meta__ArenaBlock *prev;
};

typedef struct {
    VD_Meta__ArenaBlock *curr;
    VD_Meta_AllocProc *alloc;
    void *alloc_ctx;
    size_t block_size;
} VD_Meta__Arena;

void vd_meta__arena_init(VD_Meta__Arena *arena, VD_Meta_AllocProc *alloc, void *alloc_ctx)
{
    arena->alloc = alloc;
    arena->alloc_ctx = alloc_ctx;
    arena->curr = 0;
    arena->block_size = 1024;
    VD_Meta__ArenaBlock *b = arena->alloc(0, 0, sizeof(VD_Meta__ArenaBlock) + arena->block_size, alloc_ctx);
    b->len = 0;
    b->cap = arena->block_size;
    b->next = 0;
    b->prev = 0;
    arena->curr = b;
}

void *vd_meta__arena_push(VD_Meta__Arena *arena, size_t len)
{
    if (arena->block_size < len) {
        return 0;
    }

    if (arena->curr->len + len > arena->curr->cap) {
        VD_Meta__ArenaBlock *b = arena->alloc(
            0,
            0,
            sizeof(VD_Meta__ArenaBlock) + arena->block_size,
            arena->alloc_ctx);

        b->len = 0;
        b->cap = arena->block_size;
        b->next = arena->curr;
        b->prev = 0;
        arena->curr->prev = b;
        arena->curr = b;
    }

    void *ptr = (char*)arena->curr + sizeof(VD_Meta__ArenaBlock) + arena->curr->len;
    arena->curr->len += len;

    return ptr;
}

VD_META_ALLOC_PROC(vd_meta__arena_alloc_proc) {
    if (newsize == 0) {
        return 0;
    }

    VD_Meta__Arena *arena = (VD_Meta__Arena*)c;
    void *newptr = vd_meta__arena_push(arena, newsize);
    if (!newptr) {
        return 0;
    }

    if (prevsize > 0) {
        memcpy(newptr, ptr, prevsize);
    }

    return newptr;
}

void vd_meta__arena_free(VD_Meta__Arena *arena)
{
    VD_Meta__ArenaBlock *b = arena->curr;
    while (b) {
        VD_Meta__ArenaBlock *next = b->next;
        arena->alloc(b, 0, 0, arena->alloc_ctx);
        b = next;
    }

    arena->curr = 0;
}

typedef struct {
    char                *ptr;
    size_t              len;
    size_t              cap;
    VD_Meta_AllocProc   *alloc;
    void                *alloc_ctx;
} VD_Meta__Buffer;

void vd_meta__buffer_ensure_add(VD_Meta__Buffer *buffer, size_t len)
{
    if (buffer->len + len > buffer->cap) {
        buffer->cap = buffer->cap * 2 + len;
        buffer->ptr = buffer->alloc(
            buffer->ptr,
            buffer->len,
            buffer->cap,
            buffer->alloc_ctx);
    }
}

void vd_meta__buffer_pushstr(VD_Meta__Buffer *buffer, const char *s)
{
    size_t len = strlen(s);
    vd_meta__buffer_ensure_add(buffer, len);
    memcpy(buffer->ptr + buffer->len, s, len);
    buffer->len += len;
}

void vd_meta__buffer_push_zeroes(VD_Meta__Buffer *buffer, size_t size)
{
    vd_meta__buffer_ensure_add(buffer, size);
    memset(buffer->ptr + buffer->len, 0, size);
    buffer->len += size;
}

size_t vd_meta__buffer_now(VD_Meta__Buffer *buffer)
{
    return buffer->len;
}

void vd_meta__buffer_clear(VD_Meta__Buffer *buffer)
{
    buffer->len = 0;
}

void vd_meta__buffer_free(VD_Meta__Buffer *buffer)
{
    buffer->alloc(buffer->ptr, 0, 0, buffer->alloc_ctx);
    buffer->ptr = 0;
    buffer->len = 0;
    buffer->cap = 0;
}

void *vd_meta__buffer_get(VD_Meta__Buffer *buffer, size_t offset)
{
    return buffer->ptr + offset;
}

void vd_meta__buffer_pushchar(VD_Meta__Buffer *buffer, char c)
{
    vd_meta__buffer_ensure_add(buffer, 1);
    buffer->ptr[buffer->len++] = c;
}

void vd_meta__buffer_terminate(VD_Meta__Buffer *buffer)
{
    vd_meta__buffer_ensure_add(buffer, 1);
    buffer->ptr[buffer->len] = 0;
}

static inline uint64_t vd_meta__hash(const void *key, uint32_t len, uint32_t seed)
{
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.

    const uint64_t m = 0x5bd1e995;
    const uint64_t r = 24;

    // Initialize the hash to a 'random' value

    uint64_t h = seed ^ len;

    // Mix 4 bytes at a time into the hash

    const uint8_t *data = (const uint8_t *)key;

    while (len >= 4) {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array

    switch(len) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    };

    // Do a few final mixes of the hash to ensure the last few
    // bytes are well-incorporated.

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

static VD_META_INTRUSIVE_ARRAY_ADD_PROC(vd_meta__intrusive_array_add_default_proc)
{
    VD_Meta_Registry *registry = (VD_Meta_Registry*)c;
    if (!(*array) || VD_META_ARRAY_LEN(*array) + 1 > VD_META_ARRAY_CAP(*array)) {
        *array = vd_meta__array_grow(*array, size, 1, 0, registry->alloc, registry->alloc_ctx);
    }

    memcpy(
        (char*)(*array) + VD_META_ARRAY_LEN(*array) * size,
        element,
        size);

    VD_META_ARRAY_HEADER(*array)->len++;
}

static VD_META_INTRUSIVE_ARRAY_LEN_PROC(vd_meta__intrusive_array_len_default_proc)
{
    return VD_META_ARRAY_LEN(array);
}

int vd_meta_init(VD_Meta_Registry *registry)
{
    if (registry->alloc == 0) {
        registry->alloc = vd_meta__default_alloc_proc;
        registry->alloc_ctx = 0;
    }

    if (registry->intrusive_array_props.add == 0) {
        registry->intrusive_array_props.add = vd_meta__intrusive_array_add_default_proc;
        registry->intrusive_array_props.len = vd_meta__intrusive_array_len_default_proc;
    }

    registry->table_cap = 256;
    registry->table = (VD_Meta_Descriptor**)registry->alloc(
        0,
        0,
        registry->table_cap * sizeof(VD_Meta_Descriptor*),
        registry->alloc_ctx);

    memset(registry->table, 0, registry->table_cap * sizeof(VD_Meta_Descriptor*));

    registry->type_ids_cap = 256;
    registry->type_ids = (VD_Meta__IDEntry**)registry->alloc(
        0,
        0,
        registry->type_ids_cap * sizeof(VD_Meta__IDEntry*),
        registry->alloc_ctx);

    memset(registry->type_ids, 0, registry->type_ids_cap * sizeof(VD_Meta__IDEntry*));

    registry->next_id = 1;

#if VD_META_OPTION_DEFINE_DEFAULT_TYPES
    VD_META_DEFN_TYPE(registry, int8_t, {
        .name = "int8_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(int8_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_I8,
    });
    VD_META_DEFN_TYPE(registry, uint8_t, {
        .name = "uint8_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(uint8_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_U8,
    });
    VD_META_DEFN_TYPE(registry, int16_t, {
        .name = "int16_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(int16_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_I16,
    });
    VD_META_DEFN_TYPE(registry, uint16_t, {
        .name = "uint16_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(uint16_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_U16,
    });
    VD_META_DEFN_TYPE(registry, int32_t, {
        .name = "int32_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(int32_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_I32,
    });
    VD_META_DEFN_TYPE(registry, uint32_t, {
        .name = "uint32_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(uint32_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_U32,
    });
    VD_META_DEFN_TYPE(registry, int64_t, {
        .name = "int64_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(int64_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_I64,
    });
    VD_META_DEFN_TYPE(registry, uint64_t, {
        .name = "uint64_t",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(uint64_t),
        .primitive.type = VD_META_PRIMITIVE_TYPE_U64,
    });
    VD_META_DEFN_TYPE(registry, float, {
        .name = "float",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(float),
        .primitive.type = VD_META_PRIMITIVE_TYPE_F32,
    });
    VD_META_DEFN_TYPE(registry, double, {
        .name = "double",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(double),
        .primitive.type = VD_META_PRIMITIVE_TYPE_F64,
    });
    VD_META_DEFN_TYPE(registry, char, {
        .name = "char",
        .type_class = VD_META_TYPE_PRIMITIVE,
        .size = sizeof(char),
        .primitive.type = VD_META_PRIMITIVE_TYPE_CHAR8,
    });
    VD_META_DEFN_TYPE(registry, CString, {
        .name = "CString",
        .type_class = VD_META_TYPE_STRING,
        .size = sizeof(char*),
        .string.type = VD_META_STRING_TYPE_CSTRING,
    });
#endif
    return 0;
}

VD_Meta_ID vd_meta_register(VD_Meta_Registry *registry, VD_Meta_Descriptor *descriptor)
{
    VD_Meta_Descriptor copy_desc = *descriptor;
    if (copy_desc.id.value == 0) {
        copy_desc.id.value = registry->next_id++;
    }
    copy_desc.name = strdup(descriptor->name);

    switch (copy_desc.type_class) {
        case VD_META_TYPE_PRIMITIVE:
            break;
        case VD_META_TYPE_STRING:
            break;
        case VD_META_TYPE_ARRAY:
            break;
        case VD_META_TYPE_OBJECT:
        {
            copy_desc.object.fields = (VD_Meta_Field*)registry->alloc(
                0,
                0,
                copy_desc.object.len * sizeof(VD_Meta_Field),
                registry->alloc_ctx);
            
            copy_desc.object.len = descriptor->object.len;

            memcpy(
                copy_desc.object.fields,
                descriptor->object.fields,
                descriptor->object.len * sizeof(VD_Meta_Field));

            for (int i = 0; i < copy_desc.object.len; ++i) {
                VD_Meta_Field *field = &copy_desc.object.fields[i];
                field->name = strdup(field->name);
            }
        } break;
    }

    uint64_t idhash = vd_meta__hash(&copy_desc.id, sizeof(copy_desc.id), 0x23320);
    uint64_t namehash = vd_meta__hash(descriptor->name, strlen(descriptor->name), 0x29024);

    uint64_t idindex = idhash % registry->table_cap;
    uint64_t nameindex = namehash % registry->type_ids_cap;

    VD_Meta__IDEntry identry = {
        .name = copy_desc.name,
        .id = copy_desc.id,
    };

    VD_META_ARRAY_ADD(
        registry->type_ids[nameindex],
        identry,
        registry->alloc,
        registry->alloc_ctx);

    VD_META_ARRAY_ADD(registry->table[idindex], copy_desc, registry->alloc, registry->alloc_ctx);

    return (VD_Meta_ID) { copy_desc.id.value };
}

VD_Meta_ID vd_meta_get_id(VD_Meta_Registry *registry, const char *name)
{
    uint64_t hash = vd_meta__hash(name, strlen(name), 0x29024);
    uint64_t index = hash % registry->type_ids_cap;

    for (size_t i = 0; i < VD_META_ARRAY_LEN(registry->type_ids[index]); i++) {
        VD_Meta__IDEntry *entry = &registry->type_ids[index][i];
        if (strcmp(entry->name, name) == 0) {
            return (VD_Meta_ID) { entry->id.value };
        }
    }

    return (VD_Meta_ID) { 0 };
}

VD_Meta_Descriptor *vd_meta_get_descriptor(VD_Meta_Registry *registry, VD_Meta_ID id)
{
    uint64_t hash = vd_meta__hash(&id.value, sizeof(id.value), 0x23320);
    uint64_t index = hash % registry->table_cap;

    for (size_t i = 0; i < VD_META_ARRAY_LEN(registry->table[index]); i++) {
        VD_Meta_Descriptor *desc = &registry->table[index][i];
        if (desc->id.value == id.value) {
            return desc;
        }
    }

    return 0;
}

static int vd_meta__write_json_descriptor(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_ID type,
    VD_Meta__Buffer *buffer,
    int pretty);

static int vd_meta__write_json_object(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_ID type,
    VD_Meta__Buffer *buffer,
    int pretty);

static int vd_meta__write_json_field(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_Field *field,
    VD_Meta__Buffer *buffer,
    int pretty);

static int vd_meta__write_json_field(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_Field *field,
    VD_Meta__Buffer *buffer,
    int pretty)
{
    if (field->flags & VD_META_FIELD_FLAG_FIXED_ARRAY)
    {
        if (pretty >= 0) vd_meta__buffer_pushstr(buffer, "\n");

        for (int i = 0; i < pretty; ++i) {
            vd_meta__buffer_pushchar(buffer, ' ');
        }

        vd_meta__buffer_pushstr(buffer, "[");

        if (pretty >= 0) vd_meta__buffer_pushstr(buffer, "\n");
        
        if (pretty >= 0) {
            pretty += 4;
        }

        VD_Meta_Descriptor *sub_descriptor = vd_meta_get_descriptor(registry, field->type);

        for (size_t i = 0; i < field->fixed_array.len; ++i) {
            for (int i = 0; i < pretty; ++i) {
                vd_meta__buffer_pushchar(buffer, ' ');
            }

            vd_meta__write_json_descriptor(
                registry,
                (char*)object + field->offset + i * sub_descriptor->size,
                field->type,
                buffer,
                pretty);

            if (i < field->fixed_array.len - 1) {
                vd_meta__buffer_pushstr(buffer, ",");
            }

            if (pretty >= 0) vd_meta__buffer_pushchar(buffer, '\n');
        }

        if (pretty >= 0) {
            pretty -= 4;
        }

        for (int i = 0; i < pretty; ++i) {
            vd_meta__buffer_pushchar(buffer, ' ');
        }

        vd_meta__buffer_pushstr(buffer, "]");
    } else if (field->flags & VD_META_FIELD_FLAG_INTRUSIVE_ARRAY) {
        if (pretty >= 0) vd_meta__buffer_pushstr(buffer, "\n");

        for (int i = 0; i < pretty; ++i) {
            vd_meta__buffer_pushchar(buffer, ' ');
        }

        vd_meta__buffer_pushstr(buffer, "[");

        if (pretty >= 0) vd_meta__buffer_pushstr(buffer, "\n");
        
        if (pretty >= 0) {
            pretty += 4;
        }

        uintptr_t array = *((uintptr_t *)((char *)object + field->offset));
        size_t len = registry->intrusive_array_props.len(
            (void*)array,
            registry->intrusive_array_props.c);

        VD_Meta_Descriptor *sub_descriptor = vd_meta_get_descriptor(registry, field->type);

        for (size_t i = 0; i < len; ++i) {
            for (int i = 0; i < pretty; ++i) {
                vd_meta__buffer_pushchar(buffer, ' ');
            }

            vd_meta__write_json_descriptor(
                registry,
                (char*)array + i * sub_descriptor->size,
                field->type,
                buffer,
                pretty);

            if (i < len - 1) {
                vd_meta__buffer_pushstr(buffer, ",");
            }

            if (pretty >= 0) vd_meta__buffer_pushchar(buffer, '\n');
        }

        if (pretty >= 0) {
            pretty -= 4;
        }

        for (int i = 0; i < pretty; ++i) {
            vd_meta__buffer_pushchar(buffer, ' ');
        }

        vd_meta__buffer_pushstr(buffer, "]");
    } else {
        vd_meta__write_json_descriptor(
            registry,
            (char*)object + field->offset,
            field->type,
            buffer,
            pretty);
    }
}

static int vd_meta__write_json_descriptor(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_ID type,
    VD_Meta__Buffer *buffer,
    int pretty)
{
    VD_Meta_Descriptor *descriptor = vd_meta_get_descriptor(registry, type);

    if (descriptor == 0) {
        return VD_META_DESCRIPTOR_NOT_FOUND;
    }

    switch (descriptor->type_class) {
        case VD_META_TYPE_STRING:
        {
            switch (descriptor->string.type) {
                case VD_META_STRING_TYPE_CSTRING:
                {
                    vd_meta__buffer_pushstr(buffer, "\"");
                    vd_meta__buffer_pushstr(buffer, *(char**)object);
                    vd_meta__buffer_pushstr(buffer, "\"");
                } break;
            }
        } break;
        case VD_META_TYPE_PRIMITIVE:
        {
            switch (descriptor->primitive.type) {
                case VD_META_PRIMITIVE_TYPE_I8:
                {
                    char str[32];
                    snprintf(str, 32, "%d", *(int8_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_U8:
                {
                    char str[32];
                    snprintf(str, 32, "%u", *(uint8_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_I16:
                {
                    char str[32];
                    snprintf(str, 32, "%d", *(int16_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_U16:
                {
                    char str[32];
                    snprintf(str, 32, "%u", *(uint16_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_I32:
                {
                    char str[32];
                    snprintf(str, 32, "%d", *(int32_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_U32:
                {
                    char str[32];
                    snprintf(str, 32, "%u", *(uint32_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_I64:
                {
                    char str[32];
                    snprintf(str, 32, "%lld", *(int64_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_U64:
                {
                    char str[32];
                    snprintf(str, 32, "%llu", *(uint64_t*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_F32:
                {
                    char str[32];
                    snprintf(str, 32, "%f", *(float*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_F64:
                {
                    char str[32];
                    snprintf(str, 32, "%f", *(double*)object);
                    vd_meta__buffer_pushstr(buffer, str);
                } break;
                case VD_META_PRIMITIVE_TYPE_CHAR8:
                {
                    vd_meta__buffer_pushstr(buffer, "\"");
                    vd_meta__buffer_pushchar(buffer, *(char*)object);
                    vd_meta__buffer_pushstr(buffer, "\"");
                } break;
            }
        } break;

        case VD_META_TYPE_OBJECT:
        {
            vd_meta__write_json_object(registry, object, type, buffer, pretty);
        } break;
    }

    return 0;
}

static int vd_meta__write_json_object(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_ID type,
    VD_Meta__Buffer *buffer,
    int pretty)
{
    VD_Meta_Descriptor *descriptor = vd_meta_get_descriptor(registry, type);

    if (descriptor == 0) {
        return VD_META_DESCRIPTOR_NOT_FOUND;
    }

    vd_meta__buffer_pushchar(buffer, '{');
    if (pretty >= 0) vd_meta__buffer_pushchar(buffer, '\n');

    if (pretty >= 0) {
        pretty += 4;
    }

    for (int i = 0; i < pretty; ++i) {
        vd_meta__buffer_pushchar(buffer, ' ');
    }

    // Write the type name
    vd_meta__buffer_pushstr(buffer, "\"--vd-meta-type-name--\":");
    if (pretty >= 0) vd_meta__buffer_pushchar(buffer, ' ');
    vd_meta__buffer_pushstr(buffer, "\"");
    vd_meta__buffer_pushstr(buffer, descriptor->name);
    vd_meta__buffer_pushstr(buffer, "\"");
    vd_meta__buffer_pushstr(buffer, ",");
    if (pretty >= 0) vd_meta__buffer_pushchar(buffer, '\n');

    // Write the fields
    for (int fi = 0; fi < descriptor->object.len; ++fi) {
        for (int i = 0; i < pretty; ++i) {
            vd_meta__buffer_pushchar(buffer, ' ');
        }

        VD_Meta_Field *field = &descriptor->object.fields[fi];

        vd_meta__buffer_pushstr(buffer, "\"");
        vd_meta__buffer_pushstr(buffer, field->name);
        vd_meta__buffer_pushstr(buffer, "\":");
        if (pretty >= 0) vd_meta__buffer_pushchar(buffer, ' ');

        vd_meta__write_json_field(
            registry,
            object,
            field,
            buffer,
            pretty);

        if (fi < descriptor->object.len - 1) {
            vd_meta__buffer_pushstr(buffer, ",");
        }

        if (pretty >= 0) vd_meta__buffer_pushchar(buffer, '\n');
    }

    if (pretty >= 0) {
        pretty -= 4;
        pretty = pretty < 0 ? 0 : pretty;
    }

    for (int i = 0; i < pretty; ++i) {
        vd_meta__buffer_pushchar(buffer, ' ');
    }

    vd_meta__buffer_pushchar(buffer, '}');

    return 0;
}

int vd_meta_write_json(
    VD_Meta_Registry *registry,
    void *object,
    VD_Meta_ID type,
    VD_Meta_WriteOptions *options,
    char **out_json,
    size_t *out_len)
{
    if (options->alloc == 0) {
        options->alloc = registry->alloc;
        options->alloc_ctx = registry->alloc_ctx;
    }

    VD_Meta__Buffer buffer = {
        .ptr = 0,
        .len = 0,
        .cap = 0,
        .alloc = options->alloc,
        .alloc_ctx = options->alloc_ctx,
    };

    vd_meta__write_json_object(registry, object, type, &buffer, options->pretty);
    vd_meta__buffer_terminate(&buffer);

    *out_json = buffer.ptr;
    *out_len = buffer.len;

    return 0;
}

typedef struct {
    const char *str;
    size_t len;
    size_t pos;
} VD_Meta__Slice;

static int vd_meta__slice_at_end(VD_Meta__Slice *slice)
{
    if (slice->pos >= slice->len) {
        return 1;
    }
}

static int vd_meta__slice_expect_char(VD_Meta__Slice *slice, char c)
{
    if (slice->pos >= slice->len) {
        return 0;
    }

    if (slice->str[slice->pos] != c) {
        return 0;
    }

    slice->pos++;

    return 1;
}

static int vd_meta__slice_peek_char(VD_Meta__Slice *slice, char c)
{
    if (slice->pos >= slice->len) {
        return 0;
    }

    return slice->str[slice->pos] == c;
}

static char vd_meta__slice_read_char(VD_Meta__Slice *slice)
{
    if (slice->pos >= slice->len) {
        return 0;
    }

    return slice->str[slice->pos++];
}

static int vd_meta__slice_skip_whitespace(VD_Meta__Slice *slice)
{
    while (slice->pos < slice->len) {
        if (slice->str[slice->pos] == ' ' ||
            slice->str[slice->pos] == '\t' ||
            slice->str[slice->pos] == '\n' ||
            slice->str[slice->pos] == '\r') {
            slice->pos++;
        } else {
            break;
        }
    }

    return 1;
}

#define VD_META__SLICE_COPY(x) (VD_Meta__Slice)  \
    {.str = (x)->str + (x)->pos, .len = (x)->len - (x)->pos, .pos = 0} 

#define VD_META__SLICE_SUB(s, l) (VD_Meta__Slice) \
    {.str = (s)->str + (s)->pos, .len = l, .pos = 0}

#define VD_META__SLICE_APPLY(from, to) do {\
    (to)->pos += (from)->pos; \
    } while (0)

static VD_Meta_Field *find_field(VD_Meta_Descriptor *descriptor, const char *name)
{
    for (int i = 0; i < descriptor->object.len; ++i) {
        if (strcmp(descriptor->object.fields[i].name, name) == 0) {
            return &descriptor->object.fields[i];
        }
    }

    return 0;
}

static int vd_meta__parse_json_string(
    VD_Meta__Slice *slice,
    VD_Meta__Buffer *buffer,
    const char **key)
{
    VD_Meta__Slice s = VD_META__SLICE_COPY(slice);

    if (!vd_meta__slice_expect_char(slice, '"')) {
        return 0;
    }

    char c = vd_meta__slice_read_char(slice);
    size_t start = vd_meta__buffer_now(buffer);

    while (c != '\"' && c != 0) {
        if (c == '\\') {
            char e = vd_meta__slice_read_char(slice);
            switch (e) {
                case '\\': vd_meta__buffer_pushchar(buffer, '\\'); break;
                case '\"': vd_meta__buffer_pushchar(buffer, '\"'); break;
                case '/': vd_meta__buffer_pushchar(buffer, '/'); break;
                case 'b': vd_meta__buffer_pushchar(buffer, '\b'); break;
                case 'f': vd_meta__buffer_pushchar(buffer, '\f'); break;
                case 'n': vd_meta__buffer_pushchar(buffer, '\n'); break;
                case 'r': vd_meta__buffer_pushchar(buffer, '\r'); break;
                case 't': vd_meta__buffer_pushchar(buffer, '\t'); break;
                default: vd_meta__buffer_pushchar(buffer, e); break;
            }
        } else {
            vd_meta__buffer_pushchar(buffer, c);
        }

        c = vd_meta__slice_read_char(slice);
    }

    if (c == 0) {
        return 0;
    }

    vd_meta__buffer_terminate(buffer);

    *key = (char *)vd_meta__buffer_get(buffer, start);

    VD_META__SLICE_APPLY(&s, slice);

    return 1;
}

static int vd_meta__json_skip_value(VD_Meta__Slice *slice)
{
    int num_brackets = 0;
    int num_braces = 0;
    while (1) {
        char c = vd_meta__slice_read_char(slice);
        if (c == 0) {
            return 0;
        }

        if (c == '[') {
            num_brackets++;
        } else if (c == ']') {
            num_brackets--;
        } else if (c == '{') {
            num_braces++;
        } else if (c == '}') {
            num_braces--;
        } else if (c == ',' && num_brackets == 0 && num_braces == 0) {
            return 1;
        }
    }
}

static int vd_meta__parse_json_field(
    VD_Meta_Registry *registry,
    VD_Meta_Field *field,
    VD_Meta__Arena *temp,
    VD_Meta__Buffer *btemp,
    VD_Meta__Slice *slice,
    void *object)
{
    VD_Meta__Slice s = VD_META__SLICE_COPY(slice);
    vd_meta__slice_skip_whitespace(&s);

    if (vd_meta__slice_peek_char(&s, '{')) {
        int ret = vd_meta__parse_json_object(registry, field->type, temp, btemp, &s, object);

        if (ret < 0) {
            return ret;
        }

        VD_META__SLICE_APPLY(&s, slice);
        return 0;
    } else if (vd_meta__slice_peek_char(&s, '[')) {
        // @todo: If field is an array, parse array
        return -10;
    } 

    VD_Meta_Descriptor *desc = vd_meta_get_descriptor(registry, field->type);

    switch (desc->type_class) {
        case VD_META_TYPE_PRIMITIVE: {

            switch (desc->primitive.type) {
                case VD_META_PRIMITIVE_TYPE_I8: {
                    char *end;
                    *(int8_t*)object = strtol(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_U8: {
                    char *end;
                    *(uint8_t*)object = strtoul(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_I16: {
                    char *end;
                    *(int16_t*)object = strtol(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_U16: {
                    char *end;
                    *(uint16_t*)object = strtoul(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_I32: {
                    char *end;
                    *(int32_t*)object = strtol(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_U32: {
                    char *end;
                    *(uint32_t*)object = strtoul(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_I64: {
                    char *end;
                    *(int64_t*)object = strtoll(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_U64: {
                    char *end;
                    *(uint64_t*)object = strtoull(s.str, &end, 10);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_F32: {
                    char *end;
                    *(float*)object = strtof(s.str, &end);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_F64: {
                    char *end;
                    *(double*)object = strtod(s.str, &end);
                    VD_META__SLICE_APPLY(&s, slice);
                } break;

                case VD_META_PRIMITIVE_TYPE_CHAR8: {
                    if (vd_meta__slice_expect_char(&s, '"')) {
                        *(char*)object = vd_meta__slice_read_char(&s);
                        if (vd_meta__slice_expect_char(&s, '"')) {
                            VD_META__SLICE_APPLY(&s, slice);
                        }
                    }
                } break;
            }

        } break;

        case VD_META_TYPE_STRING: {

        } break;
    }

    return 0;
}

static int vd_meta__parse_json_object(
    VD_Meta_Registry *registry,
    VD_Meta_ID type,
    VD_Meta__Arena *temp,
    VD_Meta__Buffer *btemp,
    VD_Meta__Slice *slice,
    void *out_object)
{
    VD_Meta_Descriptor *descriptor = vd_meta_get_descriptor(registry, type);
    if (descriptor == 0) {
        return VD_META_DESCRIPTOR_NOT_FOUND;
    }

    VD_Meta__Slice s = VD_META__SLICE_COPY(slice);
    vd_meta__slice_skip_whitespace(&s);

    if (!vd_meta__slice_expect_char(&s, '{')) {
        return VD_META_INVALID_JSON;
    }

    while (1) {
        vd_meta__slice_skip_whitespace(&s);

        if (vd_meta__slice_expect_char(&s, '}')) {
            running = 0;
            break;
        }

        const char *key;
        if (!vd_meta__parse_json_string(registry, &s, btemp, &key) < 0) {
            return VD_META_INVALID_JSON;
        }

        vd_meta__slice_skip_whitespace(&s);

        if (!vd_meta__slice_expect_char(&s, ':')) {
            return VD_META_INVALID_JSON;
        }

        vd_meta__slice_skip_whitespace(&s);

        VD_Meta_Field *field = find_field(descriptor, key);

        if (field == 0) {
            if (!vd_meta__json_skip_value(&s)) {
                return VD_META_INVALID_JSON;
            }

            continue;
        }
        
        int ret = vd_meta__parse_json_field(
            registry,
            field,
            temp,
            btemp,
            &s,
            (char*)out_object + field->offset);

        if (ret < 0) {
            return ret;
        }
    }

    VD_META__SLICE_APPLY(&s, slice);

    return 0;
}

static int vd_meta__json_get_object_type(
    VD_Meta_Registry *registry,
    VD_Meta_ID expected_type,
    VD_Meta_ID *actual_type,
    VD_Meta__Slice *slice,
    VD_Meta__Arena *temp,
    VD_Meta__Buffer *btemp)
{
    VD_Meta__Slice s = VD_META__SLICE_COPY(slice);
    vd_meta__slice_skip_whitespace(&s);

    if (!vd_meta__slice_expect_char(&s, '{')) {
        return VD_META_INVALID_JSON;
    }

    for (;;) {
        vd_meta__slice_skip_whitespace(&s);

        if (vd_meta__slice_expect_char(&s, '}')) {
            break;
        }

        if (vd_meta__slice_at_end(&s)) {
            return VD_META_INVALID_JSON;
        }

        const char *key;
        vd_meta__parse_json_string(&s, &btemp, &key);

        vd_meta__slice_skip_whitespace(&s);
        if (!vd_meta__slice_expect_char(&s, ':')) {
            return VD_META_INVALID_JSON;
        }

        if (strcmp(key, "--vd-meta-type-name--")) {
            vd_meta__slice_skip_whitespace(&s);

            const char *type_name;
            vd_meta__parse_json_string(&s, &btemp, &type_name);

            VD_Meta_ID id = vd_meta_get_id(registry, type_name);
            if (id.value == 0) {
                return VD_META_INVALID_TYPE;
            }

            *actual_type = id;
            if (id.value != expected_type.value) {
                return VD_META_TYPE_MISMATCH;
            }

            return 0;
        } else {

            vd_meta__slice_skip_whitespace(&s);

            vd_meta__json_skip_value(&s);

            vd_meta__slice_skip_whitespace(&s);

            vd_meta__slice_expect_char(&s, ',');
        }
    }

    if (expected_type.value != 0) {
        *actual_type = expected_type;
        return 0;
    }

    return VD_META_TYPE_NOT_FOUND;
}

int vd_meta_parse_json(
    VD_Meta_Registry *registry,
    VD_Meta_ParseOptions *options,
    const char *json,
    size_t len,
    void **out_object)
{
    if (options->alloc == 0) {
        options->alloc = registry->alloc;
        options->alloc_ctx = registry->alloc_ctx;
    }

    if (options->temp_alloc == 0) {
        options->temp_alloc = registry->alloc;
        options->temp_alloc_ctx = registry->alloc_ctx;
    }

    VD_Meta__Slice slice = {
        .str = json,
        .len = len,
        .pos = 0,
    };

    VD_Meta__Arena temp;
    vd_meta__arena_init(&temp, options->temp_alloc, options->temp_alloc_ctx);

    VD_Meta__Buffer btemp = {
        .alloc = options->temp_alloc,
        .alloc_ctx = options->temp_alloc_ctx,
        .len = 0,
        .cap = 0,
    };

    VD_Meta_ID actual_type;
    int r = vd_meta__json_get_object_type(
        registry,
        options->type,
        &actual_type,
        &slice,
        &temp,
        &btemp);

    vd_meta__buffer_clear(&btemp);

    if (r != 0) {
        return r;
    }

    VD_Meta_Descriptor *desc = vd_meta_get_descriptor(registry, actual_type);
    if (desc == 0) {
        return VD_META_DESCRIPTOR_NOT_FOUND;
    }

    *out_object = options->alloc(0, 0, desc->size, options->alloc_ctx);
    memset(*out_object, 0, desc->size);

    r = vd_meta__parse_json_object(registry, actual_type, &temp, &btemp, &slice, *out_object);

    *options->out_type = actual_type;

    vd_meta__buffer_free(&btemp);
    vd_meta__arena_free(&temp);

    return r;
}

int vd_meta_deinit(VD_Meta_Registry *registry)
{
    return 0;
}

#if VD_META_OPTION_DEFINE_DEFAULT_TYPES
VD_META_DECL_TYPE(int8_t);
VD_META_DECL_TYPE(uint8_t);
VD_META_DECL_TYPE(int16_t);
VD_META_DECL_TYPE(uint16_t);
VD_META_DECL_TYPE(int32_t);
VD_META_DECL_TYPE(uint32_t);
VD_META_DECL_TYPE(int64_t);
VD_META_DECL_TYPE(uint64_t);
VD_META_DECL_TYPE(float);
VD_META_DECL_TYPE(double);
VD_META_DECL_TYPE(char);
VD_META_DECL_TYPE(CString);
#endif

#endif // VD_META_IMPLEMENTATION
#endif // !VD_META_H