#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <string.h>  /* strlen() */
#include <errno.h>
#include <math.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static int is_json_number(const char* str, size_t len) {
    struct st {
        char lower;
        char upper;
        char next;
    };
    struct st state0[] = {
        {'\0', '-', 1},
        {'\0', '0', 2},
        {'1', '9', 3},
        {'\0', '\0', -1},
    };
    struct st state1[] = {
        {'\0', '0', 2},
        {'1', '9', 3},
        {'\0', '\0', -1},
    };
    struct st state2[] = {
        {'\0', '.', 4},
        {'\0', 'e', 5},
        {'\0', 'E', 5},
        {'\0', '\0', -1},
    };
    struct st state3[] = {
        {'0', '9', 3},
        {'\0', '.', 4},
        {'\0', 'e', 5},
        {'\0', 'E', 5},
        {'\0', '\0', -1},
    };
    struct st state4[] = {
        {'0', '9', 6},
        {'\0', '\0', -1},
    };
    struct st state5[] = {
        {'\0', '+', 7},
        {'\0', '-', 7},
        {'0', '9', 8},
        {'\0', '\0', -1},
    };
    struct st state6[] = {
        {'0', '9', 6},
        {'\0', 'e', 5},
        {'\0', 'E', 5},
        {'\0', '\0', -1},
    };
    struct st state7[] = {
        {'0', '9', 8},
        {'\0', '\0', -1},
    };
    struct st state8[] = {
        {'0', '9', 8},
        {'\0', '\0', -1},
    };
    struct st* state_table[] = { state0,state1,state2,state3,state4,state5,state6,state7,state8 };
    int cur = 0;
    for (int i = 0; i < len; i++) {
        char input = str[i];
        int next = -1;
        for (int j = 0; state_table[cur][j].next != -1; j++) {
            if (state_table[cur][j].lower && input <= state_table[cur][j].upper && state_table[cur][j].lower <= input) {
                next = state_table[cur][j].next;
                break;
            }
            else if(state_table[cur][j].upper == input) {
                next = state_table[cur][j].next;
                break;
            }
            
        }
        if (next == -1)
            return (cur == 2 || cur == 3 || cur == 6 || cur == 8)? LEPT_PARSE_ROOT_NOT_SINGULAR : LEPT_PARSE_INVALID_VALUE;
        cur = next;
    }

    return cur == 2 || cur == 3 || cur == 6 || cur == 8 ? LEPT_PARSE_OK : LEPT_PARSE_INVALID_VALUE;
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v,const char* expectStr,lept_type expectType) {
    EXPECT(c, expectStr[0]);
    int i = 1;
    while(expectStr[i]) {
        if (c->json[i - 1] != expectStr[i])
            return LEPT_PARSE_INVALID_VALUE;
        i++;
    }
    c->json += i;
    v->type = expectType;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* \TODO validate number */
    v->n = strtod(c->json, &end);
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    int ret;
    if ((ret = is_json_number(c->json, end - c->json)) != LEPT_PARSE_OK) {
        return ret;
    }
    if (errno == ERANGE && (v->n == HUGE_VAL || -v->n == HUGE_VAL)) {
        errno = 0;
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
