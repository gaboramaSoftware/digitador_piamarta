# Sistema Biométrico de Registro de Alimentación (Prototipo PAE)

![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![JavaScript](https://img.shields.io/badge/JavaScript-F7DF1E?style=flat-square&logo=javascript&logoColor=black)
![Crow](https://img.shields.io/badge/Crow-C++%20Framework-000000?style=flat-square)
![Status](https://img.shields.io/badge/Status-Prototipo%20Funcional-green?style=flat-square)

## 📋 Descripción General
Este proyecto es un **Sistema Biométrico de Registro de Alimentación** con enfoque *offline-first*. Fue desarrollado durante una práctica profesional para un establecimiento educacional adscrito al programa **PAE (Programa de Alimentación Escolar) de JUNAEB**.

El sistema soluciona la pérdida de control en la entrega de raciones mediante la identificación por huella dactilar y la emisión automatizada de tickets físicos.

---

## 🏗️ Arquitectura del Sistema
El sistema se compone de tres bloques principales:

1. **Núcleo Biométrico (C / C++)**: Comunicación de bajo nivel con el sensor **ZKTeco SLK20R** y gestión de base de datos local.
2. **Interfaz de Administración (Electron / JS Vanilla)**: Panel local para gestión de estudiantes y visualización de registros.
3. **Capa de Integración (Crow Framework)**: API HTTP en C++ que orquesta la comunicación entre hardware y la interfaz.

---

## 🚀 Funcionalidades Clave
* **Identificación Biométrica:** Validación por puntaje de coincidencia.
* **Emisión de Tickets:** Impresión automática con datos del estudiante y tipo de ración.
* **Operación Offline:** Funcionamiento garantizado sin acceso a internet.
* **Reportabilidad:** Exportación de reportes en formato CSV.

---

## 🛠️ Requisitos de Hardware
* **Sensor:** ZKTeco SLK20R.
* **Cómputo:** Intel NUC o equivalente.
* **Impresora:** Térmica o estándar reconocida por el SO.

---

## ⚙️ Decisiones Técnicas y Trade-offs
| Tecnología | Razón de elección | Observación |
| :--- | :--- | :--- |
| **Crow (C++)** | Bajo consumo y cercanía al hardware. | Excelente para rendimiento. |
| **Electron** | Velocidad de desarrollo de la interfaz. | Pragmático para prototipado rápido. |

---

## 🛤️ Evolución Proyectada
1. **Habilitación LAN:** Acceso administrativo desde la red interna del colegio.
2. **Refactor de Backend:** Mejora en la separación de responsabilidades (Node-API).
3. **Seguridad:** Implementación de autenticación con roles.

---

## 📄 Licencia
Este proyecto se presenta como portafolio técnico de práctica profesional.
