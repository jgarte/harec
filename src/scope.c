#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "expr.h"
#include "identifier.h"
#include "scope.h"
#include "util.h"

static uint32_t
name_hash(uint32_t init, const struct ident *ident)
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

struct scope_object *
scope_insert(struct scope *scope, enum object_type otype,
	struct ident *ident, struct ident *name, const struct type *type,
	struct expression *value)
{
	assert(otype == O_SCAN || !type != !value);
	struct scope_object *obj = xcalloc(1, sizeof(struct scope_object));
	obj->ident = ident;
	obj->name = name;
	obj->otype = otype;
	if (type) {
		obj->type = type;
	} else if (value) {
		obj->value = value;
		assert(otype == O_CONST);
		assert(value->type == EXPR_LITERAL);
	}
	flexible_refer(type, &obj->type);

	// Linked list
	*scope->next = obj;
	scope->next = &obj->lnext;

	// Hash map
	if (name) {
		uint32_t hash = name_hash(FNV1A_INIT, obj->name);
		struct scope_object **bucket = &scope->buckets[hash % SCOPE_BUCKETS];
		if (*bucket) {
			obj->mnext = *bucket;
		}
		*bucket = obj;
	}
	return obj;
}

struct lookup_context {
	uint32_t hash;
	struct scope_capture **captures;
	struct lookup_context *next;
};

static void
add_capture(struct lookup_context *ctx, struct scope_object *object) {
	for (; ctx; ctx = ctx->next) {
		if (!ctx->captures) {
			continue;
		}
		struct scope_capture *capture = xcalloc(1, sizeof(struct scope_capture));
		capture->object = object;
		capture->next = *ctx->captures;
		*ctx->captures = capture;
	}
}

struct scope_object *
_scope_lookup(struct lookup_context *ctx,
	struct scope *scope,
	struct ident *ident)
{
	struct scope_object *bucket = scope->buckets[ctx->hash % SCOPE_BUCKETS];
	while (bucket) {
		if (bucket->name == ident) {
			if (ctx->captures) {
				add_capture(ctx, bucket);
			}
			return bucket;
		}
		bucket = bucket->mnext;
	}

	if (!scope->parent) {
		return NULL;
	}

	struct lookup_context next_ctx = { .hash = ctx->hash };
	switch (scope->class) {
	case SCOPE_CLOSURE:
		// When traversing a closure, add its captures to the context
		next_ctx.hash = ctx->hash;
		next_ctx.next = ctx;
		next_ctx.captures = scope->captures;
		ctx = &next_ctx;
		break;
	case SCOPE_FUNC:
		// When traversing a function scope, all object references that
		// can be resolved beyond this point have global lifetimes, so
		// we can get rid of the capture lists if we started in a
		// closure.
		ctx = &next_ctx;
		break;
	default:
		break; // no-op
	}

	return _scope_lookup(ctx, scope->parent, ident);
}

struct scope_object *
scope_lookup(struct scope *scope, struct ident *ident)
{
	uint32_t hash = name_hash(FNV1A_INIT, ident);
	struct lookup_context ctx = {
		.hash = hash,
	};
	return _scope_lookup(&ctx, scope, ident);
}
