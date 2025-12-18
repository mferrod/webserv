# 🔍 REPORTE DETALLADO DE ERRORES - WebServ Evaluation

**Generado**: 18 de Diciembre de 2025, 16:20 CET  
**Script**: `evaluation.sh`  
**Score**: 21/28 (75%)

---

## 📊 MATRIZ DE RESULTADOS

```
┌─────────────────────────────────────────────────────────────────┐
│                    DESGLOSE DE RESULTADOS                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ✅ PASS:    21 tests - Funcionamiento correcto                 │
│  ⚠️  WARN:    5 tests - Advertencias / Verificación manual      │
│  ❌ FAIL:    2 tests - Funcionalidad rota                       │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  📊 TOTAL:  28 tests - Score: 75%                              │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

---

## ❌ ERRORES CRÍTICOS (FALLOS)

### ERROR #1: 404 Error Page Retorna 200 OK

**Descripción Técnica:**
```
POST:     curl http://localhost:8080/nonexistent-page-xyz
Esperado: HTTP/1.1 404 Not Found
Recibido: HTTP/1.1 200 OK
```

**Estado en Evaluación:**
```
[FAIL] 404 not returned (got: 200)
```

**¿Por Qué es Crítico?**
- Los clientes HTTP esperan 404 para recursos no encontrados
- Los buscadores confunden 200+HTML con contenido válido (problema SEO)
- Viola RFC 7231 - HTTP Semantics

**¿Dónde Buscar el Problema?**
- Archivo: `sources/HttpResponse.cpp`
- Función: `void HttpResponse::handleGet(const ServerConfig &server_config)`
- Línea aproximada: ~360-380

**Síntomas en el Log:**
```
[Response] Manejo de GET para file: <vacío>
[Response] Ruta completa al recurso: ./index.html
Reading file: ./index.html
HTTP/1.1 200  ← Debería ser 404
```

**Raíz Probable:**
El código está sirviendo `index.html` como fallback para cualquier ruta no encontrada, sin verificar primero si el archivo solicitado existe.

**Código Probable (Pseudo-código):**
```cpp
// ❌ INCORRECTO - Sirve index.html para todo
if (!file_exists(requested_file)) {
    requested_file = "index.html";  // ← Problema aquí
    return 200;  // ← Retorna 200 en vez de 404
}
```

**Corrección Requerida:**
```cpp
// ✅ CORRECTO - Verifica existencia primero
if (!file_exists(requested_file)) {
    _status_code = 404;
    makeErrorResponse();
    return;
}
```

---

### ERROR #2: CGI Retorna 500 Internal Server Error

**Descripción Técnica:**
```
POST:     curl -X POST --data "test=data" http://localhost:8080/directory/youpi.bla
Esperado: HTTP/1.1 200 OK (CGI script output)
Recibido: HTTP/1.1 500 Internal Server Error
```

**Estado en Evaluación:**
```
[FAIL] CGI not working properly (code: 500)
   CGI Response code: 500
   CGI Response: <html><body><h1>500 Internal Server Error</h1></body></html>
```

**¿Por Qué es Crítico?**
- CGI es funcionalidad principal del servidor
- `ubuntu_cgi_tester` es el validador oficial del proyecto
- Bloquea todos los tests de CGI y características avanzadas

**¿Dónde Buscar el Problema?**
- Archivo: `sources/CGI.cpp` (setup de variables de entorno)
- Archivo: `sources/HttpResponse.cpp` (routing a CGI)
- Archivo: `configs/evaluation.conf` (configuración)

**Posibles Causas (en orden de probabilidad):**

#### Causa 1: Variables de Entorno CGI Incorrectas
```
ubuntu_cgi_tester valida:
  - SCRIPT_NAME: Debe coincidir con la ruta del script
  - PATH_INFO: Debe ser la información adicional del path
  - REQUEST_METHOD: Debe ser POST
  - CONTENT_LENGTH: Debe ser correcto
  - CONTENT_TYPE: application/x-www-form-urlencoded
```

#### Causa 2: STDIN no Configurado Correctamente
```
POST data: "test=data"
En CGI, esto debe ir en STDIN con CONTENT_LENGTH correcto

Si:
  - STDIN no está connected al pipe
  - CONTENT_LENGTH no coincide con datos reales
  → ubuntu_cgi_tester retorna error (500)
```

#### Causa 3: Ruta/Permissions Incorrecta
```
La ubicación de youpi.bla:
  Ruta física: ./YoupyBanane/youpi.bla
  URI: /directory/youpi.bla
  
  El CGI necesita acceso ejecutable a ubuntu_cgi_tester
  y la ruta correcta del script
```

**Debug Recomendado:**
```bash
# Ver qué envía el CGI
./ubuntu_cgi_tester ./YoupyBanane/youpi.bla GET "" 

# Ejecutar manualmente
echo "test=data" | ./ubuntu_cgi_tester ./YoupyBanane/youpi.bla POST
```

---

## ⚠️ ADVERTENCIAS (NO FALLOS, PERO SÍ PROBLEMAS)

### ADVERTENCIA #1: PUT Retorna 405 Method Not Allowed

**Descripción:**
```
PUT:      curl -X PUT -T temp_put_file.txt http://localhost:8080/put_test/test_file.txt
Esperado: 200 OK | 201 Created | 204 No Content
Recibido: 405 Method Not Allowed
```

**En Evaluación:**
```
[WARNING] PUT request returned 405
```

**¿Por Qué Importa?**
- PUT es requerido para crear/modificar archivos vía HTTP
- La configuración permite PUT: `allowed_methods GET POST PUT DELETE`
- El error 405 indica que el servidor rechaza métodos válidos

**Causa Probable:**
1. `handlePUT()` no está implementado o retorna 405
2. El routing a `handlePUT()` no se ejecuta
3. La validación de métodos está incorrecta

**Archivo/Función:**
- `sources/HttpResponse.cpp`: Función `handlePUT()` (¿existe?)
- `sources/ServerManager.cpp`: Lógica de dispatch de métodos

---

### ADVERTENCIA #2: DELETE Retorna 404 Not Found

**Descripción:**
```
DELETE:   curl -X DELETE http://localhost:8080/uploads/delete_me.txt
Esperado: 200 OK | 204 No Content
Recibido: 404 Not Found
```

**En Evaluación:**
```
[WARNING] DELETE returned 404
```

**Análisis:**
Este error es **consecuencia en cascada** del error de PUT:
1. El test primero intenta crear un archivo con PUT
2. PUT falla (405), así que el archivo nunca se crea
3. DELETE intenta eliminar el archivo no existente
4. Recibe 404 (correcto para DELETE de no-existente)

**Causa Raíz:**
- Mismo problema que PUT - método DELETE no funciona

---

### ADVERTENCIA #3: Client Body Size Limit Retorna 405

**Descripción:**
```
POST:     curl -X POST -d "<200 bytes>" http://localhost:8080/post_body
Esperado: 413 Payload Too Large (body excede 100 bytes)
Recibido: 405 Method Not Allowed
```

**En Evaluación:**
```
[WARNING] Body limit test returned 405 (expected 413)
```

**Análisis:**
La ubicación `/post_body` no existe en `evaluation.conf`:

**En evaluation.conf:**
```
location / { allowed_methods GET; }
location /directory { allowed_methods GET POST; }
location ~ \.bla$ { allowed_methods GET POST; }
location /cgi-bin { allowed_methods GET POST; }
location /uploads { allowed_methods GET POST PUT DELETE; }
location /put_test { allowed_methods GET POST PUT DELETE; }

# ❌ Falta: location /post_body
```

**Dos Problemas:**
1. No existe location `/post_body` en config
2. Incluso si existiera, no hay handler de validación de body size

**Causa:**
- `/post_body` no está configurado → 405 (no permitido)
- O fallback al `/` que solo permite GET → 405

---

### ADVERTENCIA #4: Errno Usage - Verificación Manual

**Descripción:**
```
[WARNING] Check errno usage manually - may be acceptable context
```

**¿Qué Significa?**
El script detectó que hay uso de `errno` después de operaciones `recv()` o `send()`.

**Por Qué es Importante:**
- La especificación de 42 School probablemente prohíbe `errno` en ciertos contextos
- Esto es típicamente un punto de evaluación manual

**Verificación:**
```bash
# Buscar errno después de recv/send
grep -A 3 "recv\|send" sources/*.cpp | grep "errno"
```

---

## 📋 LISTA COMPLETA DE TESTS

### ✅ TESTS QUE PASAN (21)

1. ✅ Project compiles successfully without warnings/errors
2. ✅ webserv executable found
3. ✅ Configuration file configs/evaluation.conf found
4. ✅ YoupyBanane directory structure correct
5. ✅ ubuntu_cgi_tester found and executable
6. ✅ Uses poll() for I/O multiplexing
7. ✅ I/O multiplexing in main loop
8. ✅ poll() monitors both read (POLLIN) and write (POLLOUT) events
9. ✅ recv/send operations found in code
10. ✅ Server started successfully
11. ✅ Server responding on port 8080 (code: 200)
12. ✅ GET / returns 200 OK
13. ✅ POST on / returns 405 Method Not Allowed
14. ✅ GET /directory/ returns 200
15. ✅ Directory listing or index file served
16. ✅ GET /uploads/ returns 200 (autoindex)
17. ✅ Server survived unknown method (response: 405)
18. ✅ Server stable after CGI execution
19. ✅ Python CGI accessible
20. ✅ Server handles concurrent connections
21. ✅ Sequential requests: 100% availability

### ❌ TESTS QUE FALLAN (2)

1. ❌ 404 not returned (got: 200)
2. ❌ CGI not working properly (code: 500)

### ⚠️ ADVERTENCIAS (5)

1. ⚠️ Check errno usage manually - may be acceptable context
2. ⚠️ PUT request returned 405
3. ⚠️ DELETE returned 404
4. ⚠️ Body limit test returned 405 (expected 413)
5. ⚠️ ubuntu_tester requires manual execution

---

## 🔧 GUÍA DE INVESTIGACIÓN

### Para Investigar Error 404:

**Comando de Debug:**
```bash
# Terminal 1: Iniciar servidor
./webserv configs/evaluation.conf

# Terminal 2: Probar 404
curl -v http://localhost:8080/this_does_not_exist

# Esperar respuesta y verificar status code
```

**Revisar:**
- ✓ `sources/HttpResponse.cpp` línea ~360-390
- ✓ Buscar lógica de fallback a index.html
- ✓ Verificar cuándo se asigna `_status_code = 404`
- ✓ Ver si hay sobreescritura de status code a 200

---

### Para Investigar Error CGI:

**Comando de Debug:**
```bash
# Terminal 1: Servidor
./webserv configs/evaluation.conf

# Terminal 2: Test CGI manual
curl -v -X POST --data "test=data" http://localhost:8080/directory/youpi.bla

# Terminal 3: Ver logs del servidor (si stdout lo muestra)
tail -f server_eval.log
```

**Revisar:**
- ✓ `sources/CGI.cpp` - setup de variables de entorno
- ✓ `sources/HttpResponse.cpp` - detección de CGI
- ✓ Usar `ubuntu_cgi_tester` manualmente para debug:
  ```bash
  ./ubuntu_cgi_tester ./YoupyBanane/youpi.bla GET ""
  echo "test=data" | ./ubuntu_cgi_tester ./YoupyBanane/youpi.bla POST
  ```

---

## 📊 IMPACTO EN PUNTUACIÓN

```
Current Score: 21/28 = 75%

Potencial con fixes:
  ✅ Fix 404 error:        +1 = 22/28 (78%)
  ✅ Fix CGI:              +1 = 23/28 (82%)
  ✅ Fix PUT:              +1 = 24/28 (85%)
  ✅ Fix DELETE:           +1 = 25/28 (89%)  [cascada de PUT]
  ✅ Fix Body Size Limit:  +1 = 26/28 (92%)
  ✅ Manual Tests:         +2 = 28/28 (100%) [errno, ubuntu_tester]
```

---

## 🎯 PRÓXIMOS PASOS

### Sesión 1 (CRÍTICA):
- [ ] Fijar error 404 (retorna 200)
- [ ] Fijar CGI (retorna 500)

### Sesión 2 (IMPORTANTE):
- [ ] Implementar PUT correctamente
- [ ] Implementar DELETE correctamente
- [ ] Agregar location `/post_body` o validar body size

### Sesión 3 (REFINAMIENTO):
- [ ] Verificar errno usage
- [ ] Ejecutar ubuntu_tester manualmente
- [ ] Validar todo con evaluación final

---

## 📝 NOTAS TÉCNICAS

**Logs Disponibles:**
- `evaluation_results.log` - Resultado de cada test
- `server_eval.log` - Log del servidor durante evaluación
- `compile.log` - Output de compilación

**Herramientas de Debug:**
```bash
# Ver todo el log de evaluación
cat evaluation_results.log

# Ver log del servidor
cat server_eval.log

# Compilación limpia
make clean && make

# Ejecutar con verbose
./webserv configs/evaluation.conf
```

**Configuración de Test:**
- Puerto: 8080
- Config: `configs/evaluation.conf`
- CGI tester: `./ubuntu_cgi_tester`
- Archivos de prueba: `YoupyBanane/youpi.bla`, etc.

