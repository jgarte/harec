#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"
#include "expr.h"
#include "identifier.h"
#include "scope.h"
#include "util.h"

static uint32_t
name_hash(uint32_t init, const struct identifier *ident)
{
	return fnv1a_s(init, ident->name);
}

struct scope *
scope_push(struct scope **stack, enum scope_class class)
{
	struct scope *new = xcalloc(1, sizeof(struct scope));
	new->class = class;
	new->next = &new->objects;
	new->parent = *stack;
	*stack = new;
	return new;
}

struct scope *
scope_pop(struct scope **stack)
{
	struct scope *prev = *stack;
	assert(prev);
	*stack = prev->parent;
	return prev;
}

struct scope *
scope_lookup_class(struct scope *scope, enum scope_class class)
{
	while (scope) {
		if (scope->class == class) {
			break;
		}
		scope = scope->parent;
	}
	return scope;
}

struct scope *
scope_lookup_label(struct scope *scope, const char *label)
{
	while (scope) {
		if (scope->label && strcmp(scope->label, label) == 0) {
			break;
		}
		scope = scope->parent;
	}
	return scope;
}

void
scope_free(struct scope *scope)
{
	if (!scope) {
		return;
	}

	struct scope_object *obj = scope->objects;
	while (obj) {
		struct scope_object *next = obj->lnext;
		free(obj);
		obj = next;
	}

	free(scope);
}

void
scope_free_all(struct scopes *scopes)
{
	while (scopes) {
		struct scopes *next = scopes->next;
		scope_free(scopes->scope);
		free(scopes);
		scopes = next;
	}
}

void
scope_object_init(struct scope_object *object, enum object_type otype,
	const struct identifier *ident, const struct identifier *name,
	const struct type *type, struct expression *value,
	const struct declaration *decl)
{
	identifier_dup(&object->ident, ident);
	identifier_dup(&object->name, name);
	object->otype = otype;
	object->decl = decl;
	if (type) {
		object->type = type;
	} else if (value) {
		object->value = value;
		assert(otype == O_CONST);
		assert(value->type == EXPR_LITERAL);
	}
	flexible_refer(type, &object->type);
}

void
scope_insert_from_object(struct scope *scope, struct scope_object *object)
{
	// Linked list
	*scope->next = object;
	scope->next = &object->lnext;

	// Hash map
	uint32_t hash = name_hash(FNV1A_INIT, &object->name);
	struct scope_object **bucket = &scope->buckets[hash % SCOPE_BUCKETS];
	if (*bucket) {
		object->mnext = *bucket;
	}
	*bucket = object;
}

struct scope_object *
scope_insert(struct scope *scope, enum object_type otype,
	const struct identifier *ident, const struct identifier *name,
	const struct type *type, struct expression *value,
	const struct declaration *decl)
{
	assert(!type != !value);
	struct scope_object *o = xcalloc(1, sizeof(struct scope_object));
	scope_object_init(o, otype, ident, name, type, value, decl);
	scope_insert_from_object(scope, o);
	return o;
}

struct scope_object *
_scope_lookup(struct scope *scope,
	const struct identifier *ident,
	bool require_export)
{
	uint32_t hash = name_hash(FNV1A_INIT, ident);
	struct scope_object *bucket = scope->buckets[hash % SCOPE_BUCKETS];
	while (bucket) {
		if (!identifier_eq(&bucket->name, ident)) {
			bucket = bucket->mnext;
			continue;
		}
		const struct declaration *decl = bucket->decl;
		if (decl && require_export && !decl->exported) {
			return NULL;
		}
		return bucket;
	}

	// Exported @inline functions can only reference declarations which are
	// also exported. If we pass through an @inline function while walking
	// up the scopes, set require_export to true.
	//
	// TODO: Raise a better error message than "Unknown object 'example'"
	// in this case
	if (scope->parent) {
		struct scope *parent = scope->parent;
		require_export = require_export || parent->class == SCOPE_FUNC_INLINE;
		return _scope_lookup(scope->parent, ident, require_export);
	}

	return NULL;
}

struct scope_object *
scope_lookup(struct scope *scope, const struct identifier *ident)
{
	return _scope_lookup(scope, ident, scope->class == SCOPE_FUNC_INLINE);
}
