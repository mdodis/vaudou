#define VD_META_IMPLEMENTATION
#include "vd_meta.h"
#include "utest.h"

typedef struct {
    float x;
    float y;
    float z;
} Vector3;
VD_META_DECL_TYPE(Vector3);

typedef struct {
    Vector3 rows[3];
} Matrix3x3;

VD_META_DECL_TYPE(Matrix3x3);

typedef struct {
    int number[10];
} PhoneBookAddress;

VD_META_DECL_TYPE(PhoneBookAddress);
    
typedef struct {
    /** Intrusive array */
    PhoneBookAddress *addresses;
} PhoneBook;

VD_META_DECL_TYPE(PhoneBook);

typedef struct {
    const char  *first_name;
    const char  *middle_name;
    const char  *last_name;
    char        country_code[2];
} Contact;

VD_META_DECL_TYPE(Contact);

typedef struct {
    Contact *contacts;
} Contacts;

VD_META_DECL_TYPE(Contacts);

static void define_types(VD_Meta_Registry *registry)
{
    VD_META_DEFN_TYPE(registry, Vector3, {
        .name = "Vector3",
        .type_class = VD_META_TYPE_OBJECT,
        .size = sizeof(Vector3),
        .object = {
            .len = 3,
            .fields = (VD_Meta_Field[]){
                {
                    .name = "x",
                    .offset = offsetof(Vector3, x),
                    .type = VD_META_ID(float),
                },
                {
                    .name = "y",
                    .offset = offsetof(Vector3, y),
                    .type = VD_META_ID(float),
                },
                {
                    .name = "z",
                    .offset = offsetof(Vector3, z),
                    .type = VD_META_ID(float),
                },
            }
        }
    });

    VD_META_DEFN_TYPE(registry, Matrix3x3, {
        .name = "Matrix3x3",
        .type_class = VD_META_TYPE_OBJECT,
        .size = sizeof(Matrix3x3),
        .object = {
            .len = 1,
            .fields = (VD_Meta_Field[]){
                {
                    .name = "rows",
                    .offset = offsetof(Matrix3x3, rows),
                    .type = VD_META_ID(Vector3),
                    .flags = VD_META_FIELD_FLAG_FIXED_ARRAY,
                    .fixed_array = {
                        .len = 3,
                    }
                },
            }
        }
    });

    VD_META_DEFN_TYPE(registry, PhoneBookAddress, {
        .name = "PhoneBookAddress",
        .type_class = VD_META_TYPE_OBJECT,
        .size = sizeof(PhoneBookAddress),
        .object = {
            .len = 1,
            .fields = (VD_Meta_Field[]){
                {
                    .name = "number",
                    .offset = offsetof(PhoneBookAddress, number),
                    .type = VD_META_ID(int32_t),
                    .flags = VD_META_FIELD_FLAG_FIXED_ARRAY,
                    .fixed_array = {
                        .len = 10,
                    }
                },
            }
        }
    });

    VD_META_DEFN_TYPE(registry, PhoneBook, {
        .name = "PhoneBook",
        .type_class = VD_META_TYPE_OBJECT,
        .size = sizeof(PhoneBook),
        .object = {
            .len = 1,
            .fields = (VD_Meta_Field[]){
                {
                    .name = "addresses",
                    .offset = offsetof(PhoneBook, addresses),
                    .type = VD_META_ID(PhoneBookAddress),
                    .flags = VD_META_FIELD_FLAG_INTRUSIVE_ARRAY,
                },
            }
        }
    });

    VD_META_DEFN_TYPE(registry, Contact, {
        .name = "Contact",
        .type_class = VD_META_TYPE_OBJECT,
        .size = sizeof(Contact),
        .object = {
            .len = 4,
            .fields = (VD_Meta_Field[]) {
                {
                    .name = "first_name",
                    .offset = offsetof(Contact, first_name),
                    .type = VD_META_ID(CString),
                },
                {
                    .name = "middle_name",
                    .offset = offsetof(Contact, middle_name),
                    .type = VD_META_ID(CString),
                },
                {
                    .name = "last_name",
                    .offset = offsetof(Contact, last_name),
                    .type = VD_META_ID(CString),
                },
                {
                    .name = "country_code",
                    .offset = offsetof(Contact, country_code),
                    .type = VD_META_ID(char),
                    .flags = VD_META_FIELD_FLAG_FIXED_ARRAY,
                    .fixed_array = {
                        .len = 2,
                    }
                }
            }
        }
    });

    VD_META_DEFN_TYPE(registry, Contacts, {
        .name = "Contacts",
        .type_class = VD_META_TYPE_OBJECT,
        .size = sizeof(Contacts),
        .object = {
            .len = 1,
            .fields = (VD_Meta_Field[]) {
                {
                    .name = "contacts",
                    .offset = offsetof(Contacts, contacts),
                    .type = VD_META_ID(Contact),
                    .flags = VD_META_FIELD_FLAG_INTRUSIVE_ARRAY,
                }
            }
        }
    });
}

UTEST(vd_meta, when_registering_descriptor_then_can_retrieve_it)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);

    define_types(&registry);

    VD_Meta_ID vec3id = vd_meta_get_id(&registry, "Vector3");
    EXPECT_EQ(vec3id.value, VD_META_ID(Vector3).value);

    VD_Meta_Descriptor *vec3desc = vd_meta_get_descriptor(&registry, vec3id);
    EXPECT_NE(vec3desc, NULL);

    EXPECT_EQ(vec3desc->type_class, VD_META_TYPE_OBJECT);
    EXPECT_EQ(vec3desc->size, sizeof(Vector3));
    EXPECT_STREQ(vec3desc->name, "Vector3");
    EXPECT_EQ(vec3desc->object.len, 3);

    EXPECT_STREQ(vec3desc->object.fields[0].name, "x");
    EXPECT_EQ(vec3desc->object.fields[0].offset, offsetof(Vector3, x));
    EXPECT_EQ(vec3desc->object.fields[0].type.value, VD_META_ID(float).value);

    EXPECT_STREQ(vec3desc->object.fields[1].name, "y");
    EXPECT_EQ(vec3desc->object.fields[1].offset, offsetof(Vector3, y));
    EXPECT_EQ(vec3desc->object.fields[1].type.value, VD_META_ID(float).value);

    EXPECT_STREQ(vec3desc->object.fields[2].name, "z");
    EXPECT_EQ(vec3desc->object.fields[2].offset, offsetof(Vector3, z));
    EXPECT_EQ(vec3desc->object.fields[2].type.value, VD_META_ID(float).value);

    vd_meta_deinit(&registry);
}

UTEST(vd_meta, when_write_vec3_json_then_is_valid)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);

    define_types(&registry);

    Vector3 vec3 = {1.0f, 2.0f, 3.0f};

    char *out_json = NULL;
    size_t out_json_size = 0;
    int result = vd_meta_write_json(
        &registry,
        (void *)&vec3,
        VD_META_ID(Vector3),
        & (VD_Meta_WriteOptions) { .pretty = -1 },
        &out_json,
        &out_json_size);

    EXPECT_EQ(result, 0);
    EXPECT_STREQ("{\"--vd-meta-type-name--\":\"Vector3\",\"x\":1.000000,\"y\":2.000000,\"z\":3.000000}", out_json);

    vd_meta_deinit(&registry);
}

UTEST(vd_meta, when_write_object_with_intrusive_array_containing_fixed_array_then_is_valid)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);
    define_types(&registry);

    PhoneBook pb = {0};
    {
        PhoneBookAddress addr = { .number = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10} };
        VD_META_ARRAY_ADD(pb.addresses, addr, registry.alloc, registry.alloc_ctx);
    }

    {
        PhoneBookAddress addr = { .number = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20} };
        VD_META_ARRAY_ADD(pb.addresses, addr, registry.alloc, registry.alloc_ctx);
    }

    char *out_json = NULL;
    size_t out_json_size = 0;
    int result = vd_meta_write_json(
        &registry,
        (void *)&pb,
        VD_META_ID(PhoneBook),
        & (VD_Meta_WriteOptions) { .pretty = -1 },
        &out_json,
        &out_json_size);

    EXPECT_EQ(result, 0);
    EXPECT_STREQ(out_json, "{\"--vd-meta-type-name--\":\"PhoneBook\",\"addresses\":[{\"--vd-meta-type-name--\":\"PhoneBookAddress\",\"number\":[1,2,3,4,5,6,7,8,9,10]},{\"--vd-meta-type-name--\":\"PhoneBookAddress\",\"number\":[11,12,13,14,15,16,17,18,19,20]}]}");

    vd_meta_deinit(&registry);
}

UTEST(vd_meta, when_write_object_with_intrusive_array_of_strings_then_is_valid)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);
    define_types(&registry);

    Contacts contacts = {0};
    {
        Contact contact = {
            .first_name = "John",
            .middle_name = "Doe",
            .last_name = "Smith",
            .country_code = "US"
        };
        VD_META_ARRAY_ADD(contacts.contacts, contact, registry.alloc, registry.alloc_ctx);
    }

    {
        Contact contact = {
            .first_name = "Jane",
            .middle_name = "Doe",
            .last_name = "Smith",
            .country_code = "US"
        };
        VD_META_ARRAY_ADD(contacts.contacts, contact, registry.alloc, registry.alloc_ctx);
    }

    char *out_json = NULL;
    size_t out_json_size = 0;
    int result = vd_meta_write_json(
        &registry,
        (void *)&contacts,
        VD_META_ID(Contacts),
        & (VD_Meta_WriteOptions) {0},
        &out_json,
        &out_json_size);

    EXPECT_EQ(result, 0);

    printf("%s\n", out_json);
    
    vd_meta_deinit(&registry);
}

UTEST(vd_meta, when_parse_type_hinted_vector3_then_is_valid)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);
    define_types(&registry);

    const char *json = "{\"--vd-meta-type-name--\":\"Vector3\",\"x\":1.0,\"y\":2.0,\"z\":3.0}";

    VD_Meta_ID vec3id = vd_meta_get_id(&registry, "Vector3");
    VD_Meta_ID out_type;
    Vector3 *vec3 = NULL;
    vd_meta_parse_json(
        &registry,
        &(VD_Meta_ParseOptions) { .out_type = &out_type },
        json,
        strlen(json),
        &vec3);

    EXPECT_NE(vec3, NULL);

    EXPECT_EQ(out_type.value, vec3id.value);

    EXPECT_EQ(vec3->x, 1.0f);
    EXPECT_EQ(vec3->y, 2.0f);
    EXPECT_EQ(vec3->z, 3.0f);

    vd_meta_deinit(&registry);
}

UTEST(vd_meta, when_parse_type_hinted_unevenly_spaced_vector3_then_is_valid)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);
    define_types(&registry);

    const char *json = "{ \n   \r  \t    \"--vd-meta-type-name--\" \n   \r  \t    : \n   \r  \t    \n\n\"Vector3\"  \n   \r  \t    ,   \n   \r  \t     \"x\"\t\r: \n   \r  \t    1.0, \n   \r  \t    \t\"y\" \n   \r  \t    : \n   \r  \t    2.0, \n   \r  \t    \"z\"    \n   \r  \t      :     \n   \r  \t     3.0}";

    VD_Meta_ID vec3id = vd_meta_get_id(&registry, "Vector3");
    VD_Meta_ID out_type;
    Vector3 *vec3 = NULL;
    vd_meta_parse_json(
        &registry,
        &(VD_Meta_ParseOptions) { .out_type = &out_type },
        json,
        strlen(json),
        &vec3);

    EXPECT_NE(vec3, NULL);

    EXPECT_EQ(out_type.value, vec3id.value);

    EXPECT_EQ(vec3->x, 1.0f);
    EXPECT_EQ(vec3->y, 2.0f);
    EXPECT_EQ(vec3->z, 3.0f);

    vd_meta_deinit(&registry);
}

UTEST(vd_meta, when_parse_type_hinted_object_with_intrusive_array_of_fixed_arrays_then_is_valid)
{
    VD_Meta_Registry registry = {0};
    vd_meta_init(&registry);
    define_types(&registry);

    const char *json = "{\"--vd-meta-type-name--\":\"PhoneBook\",\"addresses\":[{\"--vd-meta-type-name--\":\"PhoneBookAddress\",\"number\":[1,2,3,4,5,6,7,8,9,10]},{\"--vd-meta-type-name--\":\"PhoneBookAddress\",\"number\":[11,12,13,14,15,16,17,18,19,20]}]}";
    size_t json_len = strlen(json);

    void *out_object = 0;
    VD_Meta_ID out_type;
    EXPECT_EQ(vd_meta_parse_json(
        &registry,
        &(VD_Meta_ParseOptions) { .out_type = &out_type },
        json,
        json_len,
        &out_object), 0);

    EXPECT_NE(out_object, NULL);
    EXPECT_EQ(out_type.value, VD_META_ID(PhoneBook).value);

    PhoneBook *pb = (PhoneBook *)out_object;

    EXPECT_EQ(VD_META_ARRAY_LEN(pb->addresses), 2);

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(pb->addresses[0].number[i], i + 1);
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(pb->addresses[1].number[i], i + 11);
    }

    vd_meta_deinit(&registry);
}