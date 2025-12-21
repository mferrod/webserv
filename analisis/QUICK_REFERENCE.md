# 🔍 TABLA DE REFERENCIA RÁPIDA - WebServ Bugs

## Resumen de Tests

| # | Test | Status | Código HTTP | Esperado | Recibido | Archivo | Línea |
|----|------|--------|-------------|----------|----------|---------|-------|
| 1 | GET /nonexistent | ❌ FAIL | 404 | 404 | 200 | HttpResponse.cpp | ~360 |
| 2 | POST /youpi.bla | ❌ FAIL | 200 | 200 | 500 | CGI.cpp | - |
| 3 | PUT /put_test | ⚠️ WARN | 201 | 200/201/204 | 405 | HttpResponse.cpp | - |
| 4 | DELETE /uploads | ⚠️ WARN | 204 | 200/204 | 404 | HttpResponse.cpp | - |
| 5 | POST body > 100 | ⚠️ WARN | 413 | 413 | 405 | HttpRequest.cpp | - |

---

## Quick Debug Commands

### 1. Test 404 Error
```bash
# Start server
./webserv configs/evaluation.conf &

# Test 404
curl -v http://localhost:8080/nonexistent
# Expected: HTTP/1.1 404 Not Found
# Actual: HTTP/1.1 200 OK ❌

# Stop server
pkill webserv
```

### 2. Test CGI
```bash
# Manual CGI test
./ubuntu_cgi_tester ./YoupyBanane/youpi.bla GET ""
# Should work without errors

# Test via HTTP
curl -X POST --data "test=data" http://localhost:8080/directory/youpi.bla
# Expected: 200 with output
# Actual: 500 Internal Server Error ❌
```

### 3. Test PUT
```bash
curl -X PUT -T /tmp/test.txt http://localhost:8080/put_test/file.txt -v
# Expected: 200 OK | 201 Created | 204 No Content
# Actual: 405 Method Not Allowed ❌
```

### 4. Test DELETE
```bash
curl -X DELETE http://localhost:8080/uploads/file.txt -v
# Expected: 200 OK | 204 No Content
# Actual: 404 Not Found ❌ (cascada de PUT)
```

---

## Files to Check

```
sources/HttpResponse.cpp (Primary)
├─ handleGet()          [Line ~304-450]  ← 404 error bug
├─ handlePUT()          [Line ~???]      ← PUT not working
├─ handleDELETE()       [Line ~???]      ← DELETE not working
└─ makeErrorResponse()  [Line ~???]      ← Error page generation

sources/CGI.cpp (Secondary)
├─ setupEnvironment()   [Line ~???]      ← CGI env vars bug
└─ execute()            [Line ~???]      ← Process execution

sources/HttpRequest.cpp
├─ parseMethod()        [Line ~???]
└─ validateBodySize()   [Line ~???]      ← Body size validation

configs/evaluation.conf
└─ location /post_body  ❌ MISSING        ← Needs to be added?
```

---

## Bug Descriptions

### 🔴 BUG #1: 404 Returns 200

**Location**: `sources/HttpResponse.cpp` → `handleGet()`

**Problem**: File not found serves index.html instead of returning 404

**Code Pattern to Look For**:
```cpp
// ❌ WRONG - This is the bug
if (file_not_found) {
    serve_index.html();  // Returns 200 OK ← WRONG
}

// ✅ CORRECT - What it should do
if (file_not_found) {
    _status_code = 404;
    makeErrorResponse();
    return;
}
```

**Test Case**: 
```
GET /this_does_not_exist → Should get 404 Not Found
Currently: 200 OK with index.html ❌
```

---

### 🔴 BUG #2: CGI Returns 500

**Location**: `sources/CGI.cpp` → `setupEnvironment()` or `sources/HttpResponse.cpp` routing

**Problem**: ubuntu_cgi_tester returns error due to bad environment variables

**Possible Causes**:
1. SCRIPT_NAME not set correctly
2. PATH_INFO not set correctly
3. STDIN not connected to process
4. CONTENT_LENGTH mismatch

**Debug**:
```bash
# Manual test
echo "test=data" | ./ubuntu_cgi_tester ./YoupyBanane/youpi.bla POST
# This should work - if it doesn't, vars are wrong
```

**Test Case**:
```
POST /directory/youpi.bla with data → Should get 200 from CGI
Currently: 500 Internal Server Error ❌
```

---

### 🟡 BUG #3: PUT Returns 405

**Location**: `sources/HttpResponse.cpp` → `handlePUT()`

**Problem**: PUT method not implemented or routing fails

**Configuration**: 
```
location /put_test {
    allowed_methods GET POST PUT DELETE;  ← PUT is allowed
}
```

**Code Pattern**:
```cpp
// Likely issue: handlePUT not implemented
if (method == "PUT") {
    // ❌ Missing implementation or returns 405
    _status_code = 405;  // Should be 200/201/204
}
```

**Test Case**:
```
PUT /put_test/file.txt → Should get 200/201/204
Currently: 405 Method Not Allowed ❌
```

---

### 🟡 BUG #4: DELETE Returns 404

**Location**: `sources/HttpResponse.cpp` → `handleDELETE()`

**Problem**: Cascading from PUT failure - file was never created

**Test Case**:
```
DELETE /uploads/delete_me.txt → Should get 200/204
Currently: 404 Not Found ❌
(But this is expected because PUT failed to create file)
```

---

### 🟡 BUG #5: Body Size Limit Returns 405

**Location**: `sources/HttpRequest.cpp` or `evaluation.conf`

**Problem**: /post_body location missing or POST not allowed

**Configuration Issue**:
```conf
# Missing in evaluation.conf:
location /post_body {
    allowed_methods POST;
    client_max_body_size 100;
}
```

**Test Case**:
```
POST /post_body with 200 bytes (exceeds 100 byte limit)
→ Should get 413 Payload Too Large
Currently: 405 Method Not Allowed ❌
```

---

## Score Impact

```
Current:  21/28 (75%)
After fixes:
  ✅ Fix 404:        +1 = 22/28 (78%)
  ✅ Fix CGI:        +1 = 23/28 (82%)
  ✅ Fix PUT:        +1 = 24/28 (85%)
  ✅ Fix DELETE:     +1 = 25/28 (89%) [cascada]
  ✅ Fix Body Size:  +1 = 26/28 (92%)
  ✅ Manual Tests:   +2 = 28/28 (100%) [errno, ubuntu_tester]
```

---

## Testing Workflow

1. **Fix 404** → Re-test: `GET /nonexistent` → should get 404
2. **Fix CGI** → Re-test: `POST /youpi.bla` → should get 200
3. **Fix PUT** → Re-test: `PUT /put_test/file.txt` → should get 201
4. **Fix DELETE** → Re-test: `DELETE /uploads/file.txt` → should get 204
5. **Fix Body Size** → Re-test: `POST /post_body with large body` → should get 413

6. **Re-run** `./evaluation.sh` to verify all changes

---

## Documentation

- 📋 `RESUMEN_EJECUTIVO.md` - Executive summary
- 📄 `INFORME_EVALUACION.md` - Full technical report
- 📄 `ERRORES_DETALLADOS.md` - Error-by-error breakdown
- 📄 `QUICK_REFERENCE.md` - This file
- 📊 `evaluation_results.log` - Raw test results

---

**Last Updated**: 18 December 2025, 16:24 CET  
**Score**: 21/28 (75%) ✅ GOOD PROJECT

