# 📊 INFORME DE EVALUACIÓN - WebServ Project

**Fecha**: 18 de Diciembre de 2025  
**Puntuación**: 21/28 (75%)  
**Estado**: BUEN PROYECTO ✅

---

## 📈 RESUMEN EJECUTIVO

El servidor WebServ ha completado la evaluación automática obteniendo un **75% de aprobación** (21 de 28 puntos). El proyecto cuenta con una arquitectura sólida basada en `poll()` para multiplexación I/O, pero presenta varios problemas en el manejo de solicitudes HTTP que impiden alcanzar la puntuación máxima.

**Áreas Funcionales:**
- ✅ **Excelente**: Compilación, Infraestructura, Estabilidad
- ✅ **Buena**: Manejo de métodos HTTP básicos, Concurrencia
- ❌ **Necesita Mejora**: Códigos de estado HTTP, Métodos HTTP avanzados, CGI

---

## 🎯 ANÁLISIS DETALLADO DE FALLOS Y ADVERTENCIAS

### 1. **FALLO CRÍTICO #1: Error 404 retorna código 200** ❌
**Criticidad**: CRÍTICA  
**Puntos Perdidos**: -1 (FAIL)  
**Test**: `GET /nonexistent-page-xyz` esperaba 404, obtuvo 200

**Análisis del Problema:**
```
Esperado:  HTTP/1.1 404 Not Found
Obtenido:  HTTP/1.1 200 OK
```

**Impacto:**
- Las páginas no encontradas se sirven como contenido válido
- Afecta SEO y comportamiento de navegadores
- Viola el estándar HTTP

**Ubicación Probable:**
- `sources/HttpResponse.cpp` en función `handleGet()` línea ~360
- Lógica de fallback a index.html sin validar existencia de archivo

**Hipótesis de Causa:**
El servidor probablemente está sirviendo `index.html` para cualquier ruta que no existe, en lugar de retornar 404. El código debe verificar si el archivo solicitado existe físicamente antes de retornar 200.

---

### 2. **FALLO CRÍTICO #2: CGI retorna 500 Internal Server Error** ❌
**Criticidad**: CRÍTICA  
**Puntos Perdidos**: -1 (FAIL)  
**Test**: `POST /directory/youpi.bla` esperaba 200, obtuvo 500

**Análisis del Problema:**
```
Respuesta CGI:
HTTP/1.1 500
Content-Type: text/html

<html><body><h1>500 Internal Server Error</h1></body></html>
```

**Impacto:**
- Los scripts CGI no funcionan correctamente
- Bloquea evaluación de funcionalidad principal (ubuntu_cgi_tester)
- Afecta todas las características basadas en CGI

**Ubicación Probable:**
- `sources/CGI.cpp` en manejo de ejecución de procesos
- `sources/HttpResponse.cpp` en enrutamiento a CGI
- Variables de entorno no configuradas correctamente para `ubuntu_cgi_tester`

**Hipótesis de Causa:**
El script `ubuntu_cgi_tester` puede estar:
1. No encontrando las variables de entorno CGI requeridas
2. No recibiendo `STDIN` correctamente en solicitudes POST
3. Recibiendo rutas o parámetros incorrectos

---

### 3. **ADVERTENCIA #1: PUT retorna 405 (Method Not Allowed)** ⚠️
**Criticidad**: MEDIA  
**Puntos Perdidos**: 0 (WARNING, no afecta puntuación pero sí funcionalidad)  
**Test**: `PUT /put_test/test_file.txt` esperaba 200/201/204, obtuvo 405

**Análisis del Problema:**
```
Esperado:   HTTP/1.1 200 OK | 201 Created | 204 No Content
Obtenido:   HTTP/1.1 405 Method Not Allowed
```

**Configuración:**
- `/put_test` está configurado con `allowed_methods GET POST PUT DELETE`
- El servidor rechaza el método PUT aunque está permitido

**Impacto:**
- No se pueden crear/actualizar archivos vía HTTP PUT
- Requisito para evaluación de métodos HTTP completos

**Ubicación Probable:**
- `sources/HttpResponse.cpp` en función que valida métodos permitidos
- `sources/HttpRequest.cpp` en parseo del método HTTP
- `sources/ServerManager.cpp` en lógica de dispatch de métodos

**Hipótesis de Causa:**
1. El método PUT podría no estar implementado en `handlePUT()`
2. La validación de métodos permitidos podría estar fallando
3. El match entre la location `/put_test` y la ruta puede no ser correcto

---

### 4. **ADVERTENCIA #2: DELETE retorna 404** ⚠️
**Criticidad**: MEDIA  
**Puntos Perdidos**: 0 (WARNING)  
**Test**: `DELETE /uploads/delete_me.txt` esperaba 200/204, obtuvo 404

**Análisis del Problema:**
```
Esperado:   HTTP/1.1 200 OK | 204 No Content
Obtenido:   HTTP/1.1 404 Not Found
```

**Configuración:**
- `/uploads` tiene `allowed_methods GET POST PUT DELETE`
- El archivo `delete_me.txt` debería haber sido creado antes pero falló

**Impacto:**
- DELETE no funciona correctamente
- Requisito para operaciones CRUD completas

**Ubicación Probable:**
- Fallo en cascada del intento de PUT (causa que DELETE no pueda completarse)
- `sources/HttpResponse.cpp` en función `handleDelete()`

**Hipótesis de Causa:**
Este fallo es consecuencia del problema con PUT. Como PUT falla (405), el archivo nunca se crea, y DELETE no encuentra nada que eliminar.

---

### 5. **ADVERTENCIA #3: Body Size Limit retorna 405 en lugar de 413** ⚠️
**Criticidad**: MEDIA  
**Puntos Perdidos**: 0 (WARNING)  
**Test**: `POST` con 200 bytes a `/post_body` esperaba 413 (Payload Too Large), obtuvo 405

**Análisis del Problema:**
```
Esperado:   HTTP/1.1 413 Payload Too Large
Obtenido:   HTTP/1.1 405 Method Not Allowed
```

**Configuración del Servidor:**
- `client_max_body_size 100` (límite de 100 bytes)
- El body enviado era de 200 bytes (2x el límite)
- La ruta `/post_body` probablemente no existe en la configuración

**Impacto:**
- No se valida correctamente el límite de tamaño de body
- Violación de requisitos HTTP/1.1

**Ubicación Probable:**
- `sources/HttpRequest.cpp` en parseo del body
- `sources/HttpResponse.cpp` en validación de tamaño de body
- Falta la ruta `/post_body` en configuración

**Hipótesis de Causa:**
1. El server devuelve 405 porque `/post_body` no existe o no tiene POST permitido
2. La validación de `client_max_body_size` puede ocurrir después de esta validación
3. El orden de validaciones puede estar incorrecto

---

### 6. **ADVERTENCIA #4: errno usage - Verificación Manual Requerida** ⚠️
**Criticidad**: BAJA  
**Puntos Perdidos**: 0 (WARNING)  
**Nota**: El script de evaluación sugiere revisión manual de uso de `errno` después de `recv()`/`send()`

**Contexto:**
- La especificación de 42 School probablemente prohíbe el uso de `errno` en ciertos contextos
- Esto es un punto normalmente verificado por evaluadores humanos

**Impacto:**
- Potencial descalificación manual si se usa `errno` de forma prohibida

---

## ✅ LO QUE FUNCIONA BIEN

### Compilación y Estructura (5/5 ✅)
- ✅ Proyecto compila sin errores ni warnings
- ✅ Executable `webserv` generado correctamente
- ✅ Config file `evaluation.conf` existe
- ✅ Estructura de directorios `YoupyBanane` correcta
- ✅ `ubuntu_cgi_tester` disponible

### Infraestructura I/O (5/5 ✅)
- ✅ Usa `poll()` para multiplexación I/O
- ✅ `poll()` está en el main loop
- ✅ Monitorea `POLLIN` (lectura) y `POLLOUT` (escritura)
- ✅ Operaciones `recv/send` implementadas
- ⚠️ Verificación manual necesaria para `errno` usage

### Configuración HTTP Básica (5/5 ✅)
- ✅ Server inicia correctamente en puerto 8080
- ✅ Server responde a requests
- ✅ `GET /` retorna 200 OK
- ✅ `POST /` (sin permitir) retorna 405 Method Not Allowed
- ✅ Server sobrevive a métodos desconocidos (405 response)

### Métodos HTTP Parcial (3/5 ⚠️)
- ✅ `GET` funciona correctamente (excepto 404)
- ✅ `GET /directory/` retorna 200
- ✅ `GET /uploads/` retorna 200 (autoindex)
- ❌ `PUT` retorna 405 (debería 200/201/204)
- ❌ `DELETE` retorna 404 (debería 200/204)

### Autoindex y Directorios (5/5 ✅)
- ✅ Autoindex funciona para `/directory/`
- ✅ Autoindex funciona para `/uploads/`
- ✅ Se sirve contenido de directorios
- ✅ Links y navegación correcta

### Estabilidad y Concurrencia (5/5 ✅)
- ✅ Server maneja 20 conexiones concurrentes
- ✅ 30/30 requests secuenciales exitosos (100%)
- ✅ Server no se cae bajo carga
- ✅ Server no se cae tras métodos desconocidos
- ✅ Server estable después de CGI

### CGI Parcial (1/3 ⚠️)
- ❌ CGI `.bla` retorna 500
- ✅ Python CGI `/cgi-bin/test.py` accesible (200)
- ⚠️ ubuntu_tester requiere ejecución manual

---

## 📝 RESUMEN DE PUNTOS

| Categoría | Tests | Passou | Puntos | Estado |
|-----------|-------|--------|--------|--------|
| Prerequisites | 5 | 5 | 5/5 | ✅ |
| Code Inspection | 5 | 4 | 4/5 | ⚠️ |
| Config Testing | 5 | 4 | 4/5 | ❌ |
| HTTP Methods | 8 | 5 | 5/8 | ❌ |
| CGI Testing | 3 | 1 | 1/3 | ❌ |
| Stress Testing | 2 | 2 | 2/2 | ✅ |
| Ubuntu Tester | 1 | 0 | 0/1 | ⚠️ |
| **TOTAL** | **28** | **21** | **21/28** | **75%** |

---

## 🔧 DIAGNÓSTICO TÉCNICO

### Problema Raíz #1: Códigos de Estado HTTP Incorrectos
**Síntomas:**
- 404 retorna 200
- PUT retorna 405
- DELETE retorna 404
- Body Limit retorna 405

**Causa Probable:**
- Manejo de estado incorrecto en `HttpResponse::handleGet()` y métodos asociados
- Posible corrupción o sobreescritura de `_status_code`
- Lógica de enrutamiento y validación de métodos defectuosa

**Archivos Afectados:**
- `sources/HttpResponse.cpp` (líneas ~304-450)
- `sources/HttpRequest.cpp` (parseo de métodos)
- `sources/ServerManager.cpp` (dispatch)

---

### Problema Raíz #2: CGI no Funciona
**Síntomas:**
- POST a `/directory/youpi.bla` retorna 500
- `ubuntu_cgi_tester` no recibe datos correctos

**Causa Probable:**
- Variables de entorno CGI no configuradas correctamente
- Ruteo a CGI fallido
- `ubuntu_cgi_tester` requiere configuración específica de variables

**Archivos Afectados:**
- `sources/CGI.cpp` (setup de variables de entorno)
- `sources/HttpResponse.cpp` (detección y routing a CGI)
- `configs/evaluation.conf` (configuración de CGI)

---

## 🚀 PRÓXIMOS PASOS RECOMENDADOS

### Prioridad CRÍTICA (Necesarios para pasar):
1. **Fijar 404 Error Handling** - Verificar que archivos no existentes retornan 404, no 200
2. **Fijar CGI** - Debug de variables de entorno y ejecución de CGI scripts

### Prioridad ALTA (Completan la evaluación):
3. **Implementar PUT Method** - Debe crear/actualizar archivos
4. **Implementar DELETE Method** - Debe eliminar archivos
5. **Validar Client Body Size** - Debe retornar 413 cuando se exceda límite

### Prioridad MEDIA (Refinamiento):
6. **Verificar errno usage** - Asegurar cumplimiento de normas 42 School
7. **Ejecutar ubuntu_tester manualmente** - Para validación completa de CGI

---

## 📋 NOTAS FINALES

- **Arquitectura**: Sólida, con buena base de multiplexación I/O
- **Estabilidad**: Excelente bajo carga concurrente
- **Completitud**: Parcial - métodos HTTP no totalmente implementados
- **Calidad de Código**: Requiere auditoría de manejo de estados HTTP
- **Próximas Sesiones**: Enfocarse en problemas de raíz listados arriba

