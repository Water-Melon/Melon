## Script development

This article will introduce how to develop a dynamic extension library for scripting languages, and also give various functions required for development.



### Header file

```c
#include "mln_lang.h"
```



### Module

`lang`



### Development steps

The development steps of dynamic extension library is as follows:

1. Construct a dynamic library with a function named `init`, where the function prototype of `init` is:

   ```c
   mln_lang_var_t *init(mln_lang_ctx_t *);
   ```

   `mln_lang_var_t` is the type of the return value of the function in the script task. For the creation method, please refer to `mln_lang.h` and the implementation of various built-in functions of melang.

   Developers can load various script functions, collections, variables, etc. in the `init` function. If the function returns `nil`, this is similar to step 4 of the built-in library.

   The `mln_lang_var_t` return value in `init` is returned at the end of the function `import` call.

2. When using, call the `import` function in the script to import this dynamic library, and the program will call the `init` function to load the resources to be loaded in the library into the current scope of the script.



### Functions/Macros

Developers can not only use the functions given here, but also the functions introduced in the previous article. For example, the implementation of the `eval` function in Melang.



### Structures

```c
struct mln_lang_var_s {
    mln_lang_ctx_t                  *ctx;//Script task structure
    mln_lang_var_type_t              type;//Is it a reference type
    mln_string_t                    *name;//Variable name, anonymous variables will be set to NULL
    mln_lang_val_t                  *val;//variable value structure
    mln_lang_set_detail_t           *in_set;//Whether it belongs to a Set structure (ie, class definition)
    mln_lang_var_t                  *prev;
    mln_lang_var_t                  *next;
    mln_lang_var_t                  *prev;
    mln_lang_var_t                  *next;
    mln_uauto_t                      ref;//reference counter
};

struct mln_lang_val_s {
    mln_lang_ctx_t                  *ctx;//Script task structure
    struct mln_lang_val_s           *prev;
    struct mln_lang_val_s           *next;
    union {
        mln_s64_t                i;
        mln_u8_t                 b;
        double                   f;
        mln_string_t            *s;
        mln_lang_object_t       *obj;
        mln_lang_func_detail_t  *func;
        mln_lang_array_t        *array;
        mln_lang_funccall_val_t *call;
    } data; //value data
    mln_s32_t                        type;//data type
    mln_u32_t                        ref;//The number of times the value structure is referenced
    mln_lang_val_t                  *udata;//for reactive programming
    mln_lang_func_detail_t          *func;//for reactive programming
    mln_u32_t                        not_modify:1;//read-only or not
};

struct mln_lang_symbol_node_s {
    mln_string_t                    *symbol;//symbol string
    mln_lang_ctx_t                  *ctx;//Script task structure
    mln_lang_symbol_type_t           type;//Symbol type: Class (M_LANG_SYMBOL_SET) or variable (M_LANG_SYMBOL_VAR)
    union {
        mln_lang_var_t          *var;
        mln_lang_set_detail_t   *set;
    } data;
    mln_uauto_t                      layer;//The scope layer to which it belongs
    mln_lang_hash_bucket_t          *bucket;//Symbol table bucket structure
    struct mln_lang_symbol_node_s   *prev;
    struct mln_lang_symbol_node_s   *next;
    struct mln_lang_symbol_node_s   *scope_prev;
    struct mln_lang_symbol_node_s   *scope_next;
};
```

In a script, these first two structures are the most important structures in the script. You can see that `mln_lang_var_t` is a wrapper around `mln_lang_val_t`. `mln_lang_val_t` is used to store specific values, strings, etc. `mln_lang_var_t` is generally used as a package structure for function parameters, return values, and array element values.

The reason for encapsulating a layer is for two aspects:

1. Function parameters reference
2. Mapping variable names to variable values

We can call the former a variable structure and the latter a value structure.

The types of value structures include the following:

- `M_LANG_VAL_TYPE_NIL` null value
- `M_LANG_VAL_TYPE_INT` integer
- `M_LANG_VAL_TYPE_BOOL` boolean
- `M_LANG_VAL_TYPE_REAL` real
- `M_LANG_VAL_TYPE_STRING` string
- `M_LANG_VAL_TYPE_OBJECT` object
- `M_LANG_VAL_TYPE_FUNC` function definition
- `M_LANG_VAL_TYPE_ARRAY` array
- `M_LANG_VAL_TYPE_CALL` function call



#### mln_lang_var_new

```c
mln_lang_var_t *mln_lang_var_new(mln_lang_ctx_t *ctx, mln_string_t *name, mln_lang_var_type_t type, mln_lang_val_t *val, mln_lang_set_detail_t *in_set);
```

Description: Create a variable structure. The meanings of the parameters are as follows:

- `ctx` script structure
- `name` variable name, or `NULL` if none
- `type` variable type, reference (`M_LANG_VAR_REFER`) or copy (`M_LANG_VAR_NORMAL`).
- The value structure corresponding to `val`.
- Whether `in_set` belongs to a Set (class definition), if not, set `NULL`.

Return value: return the structure pointer if successful, otherwise return `NULL`



#### mln_lang_var_free

```c
void mln_lang_var_free(void *data);
```

Description: Frees resources for pointer `data` of type `mln_lang_var_t`, including its internal value structure.

Return value: none



#### mln_lang_var_ref

```c
mln_lang_var_ref(var)
```

Description: Creates a reference to the variable structure. During freeing, if the reference is greater than 1, the variable structure is not actually freed.

Return value: pointer of type `mln_lang_var_t`



#### mln_lang_var_val_get

```c
mln_lang_var_val_get(var)
```

Description: Get the value structure of the pointer `var` of type `mln_lang_var_t`.

Return value: pointer of type `mln_lang_val_t`



#### mln_lang_val_not_modify_set

```c
mln_lang_val_not_modify_set(val)
```

Description: Set the `mln_lang_val_t` type pointer `val` to be immutable.

Return value: none



#### mln_lang_val_not_modify_isset

```c
mln_lang_val_not_modify_isset(val)
```

Description: Determine whether the pointer `val` of type `mln_lang_val_t` is immutable.

Return value: unchangeable returns `non-0`, otherwise returns `0`



#### mln_lang_val_not_modify_reset

```c
mln_lang_val_not_modify_reset(val)
```

Description: Set the `mln_lang_val_t` type pointer `val` to be modifiable.

Return value: none



#### mln_lang_var_dup

```c
mln_lang_var_t *mln_lang_var_dup(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
```

Description: duplicate `var`. For array and object type variables, only reference is not copied. The replica uses the memory pool structure in ctx for memory allocation.

Return value: variable structure pointer.



#### mln_lang_var_value_set

```c
int mln_lang_var_value_set(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
```

Description: Assign the value in `src` to `dest`. If there is a value in `dest`, the value will be released. For arrays and objects, only references, no copies. For strings, this function makes an exact copy.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_var_value_set_string_ref

```c
int mln_lang_var_value_set_string_ref(mln_lang_ctx_t *ctx, mln_lang_var_t *dest, mln_lang_var_t *src);
```

Description: Assign the value in `src` to `dest`. If there is a value in `dest`, the value will be released. For arrays and objects, only references, no copies. For strings, this function only references and does not copy.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_var_val_type_get

```c
mln_s32_t mln_lang_var_val_type_get(mln_lang_var_t *var);
```

Description: Get the type of the value of the variable. For the type definition, see the relevant structure section.

Return value: type value



#### mln_lang_ctx_global_var_add

```c
int mln_lang_ctx_global_var_add(mln_lang_ctx_t *ctx, mln_string_t *name, void *val, mln_u32_t type);
```

Description: Add an outermost (also global) variable to the script task `ctx`. The variable name is `name`, the variable value is `val`, and the type of the value is `type`. `type` satisfies the type defined in the relevant structure section. If it is a string type, the new variable will be referenced by `mln_string_ref`, so pay attention to the possible problems caused when `val` is allocated on the stack.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_var_create_call

```c
mln_lang_var_t *mln_lang_var_create_call(mln_lang_ctx_t *ctx, mln_lang_funccall_val_t *call);
```

Description: Creates a variable of type function call. This variable is only used in the script core and is mentioned here only by the way.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_nil

```c
mln_lang_var_t *mln_lang_var_create_nil(mln_lang_ctx_t *ctx, mln_string_t *name);
```

Description: Creates a variable of empty type. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_obj

```c
mln_lang_var_t *mln_lang_var_create_obj(mln_lang_ctx_t *ctx, mln_lang_set_detail_t *in_set, mln_string_t *name);
```

Description: Creates a variable of type object. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack. `in_set` is the class (Set) to which the object belongs. This function only refers to `in_set` without copying it completely.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_true

```c
mln_lang_var_t *mln_lang_var_create_true(mln_lang_ctx_t *ctx, mln_string_t *name);
```

Description: Creates a boolean true variable. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_false

```c
mln_lang_var_t *mln_lang_var_create_false(mln_lang_ctx_t *ctx, mln_string_t *name);
```

Description: Creates a boolean false variable. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_int

```c
mln_lang_var_t *mln_lang_var_create_int(mln_lang_ctx_t *ctx, mln_s64_t off, mln_string_t *name);
```

Description: Create a variable of integer type. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_real

```c
mln_lang_var_t *mln_lang_var_create_real(mln_lang_ctx_t *ctx, double f, mln_string_t *name);
```

Description: Creates a variable of type real. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_bool

```c
mln_lang_var_t *mln_lang_var_create_bool(mln_lang_ctx_t *ctx, mln_u8_t b, mln_string_t *name);
```

Description: Creates a boolean variable. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack. Boolean false when `b` is `0`, boolean true otherwise.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_string

```c
mln_lang_var_t *mln_lang_var_create_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name);
```

Description: Creates a variable of empty type. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack. This function makes an exact copy of `s`.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_ref_string

```c
mln_lang_var_t *mln_lang_var_create_ref_string(mln_lang_ctx_t *ctx, mln_string_t *s, mln_string_t *name);
```

Description: Creates a variable of empty type. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack. This function will only refer to `s` with `mln_string_ref`, so pay attention to the effect of `s` for stack allocated memory.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_var_create_array

```c
mln_lang_var_t *mln_lang_var_create_array(mln_lang_ctx_t *ctx, mln_string_t *name);
```

Description: Creates an array type variable. If the variable does not require a variable name, `name` is set to `NULL`. Note that name will be referenced by `mln_string_ref`, so you need to pay attention to the impact of `name` for memory allocated on the stack. After creation, it is an empty array.

Return value: return the variable type pointer if successful, otherwise return `NULL`



#### mln_lang_condition_is_true

```
int mln_lang_condition_is_true(mln_lang_var_t *var);
```

escription: Determines whether the value of the `var` variable is logically true.

Return value: `1` if true, `0` otherwise



#### mln_lang_ctx_set_ret_var

```c
void mln_lang_ctx_set_ret_var(mln_lang_ctx_t *ctx, mln_lang_var_t *var);
```

Description: Set the return value of the current script task `ctx` to `var`. `var` is referenced here rather than copied.

Return value: none



#### mln_lang_val_new

```c
mln_lang_val_t *mln_lang_val_new(mln_lang_ctx_t *ctx, mln_s32_t type, void *data);
```

Description: Create a value structure. See the relevant structure section for the `type` type. `data` is different according to `type`, so its type is also different. For the specific type, please refer to the type of the community part of the structure in the related structure. But it should be noted that this parameter is a pointer type, that is, the integer type is `mln_s64_t *` rather than `mln_s64_t` type cast.

Return value: return value structure pointer if successful, otherwise return `NULL`



#### mln_lang_val_free

```c
void mln_lang_val_free(mln_lang_val_t *val);
```

Description: Frees the value structure.

Return value: none



#### mln_lang_symbol_node_search

```c
mln_lang_symbol_node_t *mln_lang_symbol_node_search(mln_lang_ctx_t *ctx, mln_string_t *name, int local);
```

Description: Query the symbol table of script task `ctx` for the symbol named `name`. `local` is `non-0` if the query is only made within the current function scope, `0` otherwise.

Return value: Returns the symbolic structure pointer if it exists, otherwise returns `NULL`



#### mln_lang_symbol_node_join

```c
int mln_lang_symbol_node_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data);
```

Description: Add `data` of type `type` to the symbol table. The value of `type` is:

- `M_LANG_SYMBOL_VAR` - variable
- `M_LANG_SYMBOL_SET` - class

Depending on the `type`, the type of `data` is different:

- `mln_lang_var_t*`
- `mln_lang_set_detail_t *`

When processed by this function, `data` is referenced directly and not copied.

**Note**: Variables must have variable names.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_symbol_node_upper_join

```c
int mln_lang_symbol_node_upper_join(mln_lang_ctx_t *ctx, mln_lang_symbol_type_t type, void *data);
```

Description: Add `data` of type `type` to the symbol table of the previous scope. If it is already the outermost layer, add it to the outermost symbol table. The value of `type` is:

- `M_LANG_SYMBOL_VAR` - variable
- `M_LANG_SYMBOL_SET` - class

Depending on the `type`, the type of `data` is different:

- `mln_lang_var_t*`
- `mln_lang_set_detail_t *`

When processed by this function, `data` is referenced directly and not copied.

**Note**: Variables must have variable names.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_func_detail_new

```c
mln_lang_func_detail_t *mln_lang_func_detail_new(mln_lang_ctx_t *ctx, mln_lang_func_type_t type, void *data, mln_lang_exp_t *exp, mln_lang_exp_t *closure);
```

Description: Create a function definition. Parameter meaning:

- `ctx` script structure
- `type` function type: internal functions (`M_FUNC_INTERNAL`) and functions defined in scripts (`M_FUNC_EXTERNAL`)
- The specific processing part of the `data` function. When `type` is `M_FUNC_INTERNAL`, `data` type is `mln_lang_internal`. Otherwise the type is `mln_lang_stm_t *`, which is the statement structure in the abstract syntax tree structure.
- `exp` is a parameter list expression structure. When using C to construct the internal function by yourself, set this parameter to `NULL`, and then construct the parameter list by yourself.

Return value: If successful, return the function definition structure pointer, otherwise return `NULL`



#### mln_lang_func_detail_free

```c
void mln_lang_func_detail_free(mln_lang_func_detail_t *lfd);
```

Description: Free the function definition structure.

Return value: none



#### mln_lang_set_detail_new

```c
mln_lang_set_detail_t *mln_lang_set_detail_new(mln_alloc_t *pool, mln_string_t *name);
```

Description: Create a class (Set) definition structure. `name` cannot be `NULL`, and this function will make a full copy of it.

Return value: If successful, return the class definition structure pointer, otherwise return `NULL`



#### mln_lang_set_detail_free

```c
void mln_lang_set_detail_free(mln_lang_set_detail_t *c);
```

Description: Frees the class definition structure.

Return value: none



#### mln_lang_set_detail_self_free

```c
void mln_lang_set_detail_self_free(mln_lang_set_detail_t *c);
```

Description: Frees the class definition structure. The difference from `mln_lang_set_detail_free` is that this function will first release the member red-black tree in the class definition, while `mln_lang_set_detail_free` is to judge the reference count, if the reference is greater than 1, the red-black tree will not be released.

Return value: none



#### mln_lang_set_member_search

```c
mln_lang_var_t *mln_lang_set_member_search(mln_rbtree_t *members, mln_string_t *name);
```

Description: Finds if a member named `name` exists in a member red-black tree. This function can be used for both class and object member query, because both structures contain member red-black tree structures.

Return value: if it exists, return the variable structure pointer of the member, otherwise return `NULL`



#### mln_lang_set_member_add

```c
int mln_lang_set_member_add(mln_alloc_t *pool, mln_rbtree_t *members, mln_lang_var_t *var);
```

Description: Add a member `var` to the red-black tree of class members.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_object_add_member

```c
int mln_lang_object_add_member(mln_lang_ctx_t *ctx, mln_lang_object_t *obj, mln_lang_var_t *var);
```

Description: Add a member `var` to the object.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_array_new

```c
 mln_lang_array_t *mln_lang_array_new(mln_lang_ctx_t *ctx);
```

Description: Create a new array structure.

Return value: return array structure pointer if successful, otherwise return `NULL`



#### mln_lang_array_free

```c
void mln_lang_array_free(mln_lang_array_t *array);
```

Description: Free the array structure, if the array reference count is greater than 1, only decrement the reference count.

Return value: none



#### mln_lang_array_get

```c
mln_lang_var_t *mln_lang_array_get(mln_lang_ctx_t *ctx, mln_lang_array_t *array, mln_lang_var_t *key);
```

Description: Get the element of `key` in the following table in the array `array`. If `key` does not exist, create a new array element. If `key` is `NULL`, the array subscript will be appended by itself. `key` is a variable structure, that is, `key` can be either an integer, a string, an object, etc.

Return value: If successful, return the variable structure pointer of the new element or the existing element, otherwise return `NULL`



#### mln_lang_array_elem_exist

```c
int mln_lang_array_elem_exist(mln_lang_array_t *array, mln_lang_var_t *key);
```

Description: Determines whether the element with subscript `key` exists in the array `array`.

Return value: returns `1` if exists, otherwise returns `0`



#### mln_lang_ctx_resource_register

```c
int mln_lang_ctx_resource_register(mln_lang_ctx_t *ctx, char *name, void *data, mln_lang_resource_free free_handler);

typedef void (*mln_lang_resource_free)(void *data);
```

Description: Register script task level resources and their cleanup functions with script task `ctx`. For example, the messages of the message queue need to be managed at the task level. For example, when the current task exits actively or passively, the resources used by the task need to be cleaned up uniformly. See the usage in `Melon/melang/msgqueue`. `name` is the resource name, `data` is the resource pointer, `free_handler` is the release function, and the parameter of the release function is the resource pointer `data`.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_ctx_resource_fetch

```c
void *mln_lang_ctx_resource_fetch(mln_lang_ctx_t *ctx, const char *name);
```

Description: Get the resource named `name` registered in the script task `ctx`.

Return value: Returns the resource pointer if it exists, otherwise returns `NULL`



#### mln_lang_resource_register

```c
int mln_lang_resource_register(mln_lang_t *lang, char *name, void *data, mln_lang_resource_free free_handler);

typedef void (*mln_lang_resource_free)(void *data);
```

Description: Register a resource `data` named `name` and its cleanup function `free_handler` with the script management structure `lang`. The meaning of this registration is that the resource will be used across scripting tasks, such as network communication, allowing the socket to be handled by a new coroutine alone.

Return value: return `0` if successful, otherwise return `-1`



#### mln_lang_resource_cancel

```c
void mln_lang_resource_cancel(mln_lang_t *lang, const char *name);
```

Description: Unregister the resource named `name` from the script management structure and free the resource.

Return value: none



#### mln_lang_resource_fetch

```c
void *mln_lang_resource_fetch(mln_lang_t *lang, const char *name);
```

Description: Get a resource structure named `name` from the script management structure `lang`.

Return value: Returns the resource pointer if it exists, otherwise returns `NULL`



#### mln_lang_ctx_suspend

```c
void mln_lang_ctx_suspend(mln_lang_ctx_t *ctx);
```

Description: Suspend script task `ctx` to wait. `mln_lang_mutex_lock` must be called before calling this function.

Return value: none



#### mln_lang_ctx_continue

```c
void mln_lang_ctx_continue(mln_lang_ctx_t *ctx);
```

Description: Put the script task `ctx` back in the execution queue to continue execution. `mln_lang_mutex_lock` must be called before calling this function.

Return value: none



#### mln_lang_func_detail_arg_append

```c
void mln_lang_func_detail_arg_append(mln_lang_func_detail_t *func, mln_lang_var_t *var);
```

Description: Append argument `var` to the argument list of function definition `func`.

Return value: none



### 示例

The implementation of the function can refer to the content in `Melang/lib_src/aes`, which is extremely concise.

The part of the class implementation can refer to the content in `Melang/lib_src/file`.
