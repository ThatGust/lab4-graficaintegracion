# Actividad 6: Integración final

Aplicación en C++ con OpenGL (GLUT) que reúne en un solo programa lo desarrollado en las actividades anteriores. Permite crear varios polígonos con el mouse y rasterizar su contorno con **Bresenham**. También permite rotarlos, escalarlos y trasladarlos con **matrices homogéneas**, y rellenarlos con el algoritmo **Scan-Line usando ET y EAT**.

- **Curso:** Computación Gráfica, Semana 4
- **Autor:** Guillermo Cano Luque
- **Entorno:** Windows + [ZinjaI](http://zinjai.sourceforge.net/) (MinGW GCC 6.3 + freeglut)

![Tres polígonos rellenados con colores distintos](capturas/08_traslacion_relleno.png)

## Contenido

1. [Código reutilizado](#1-código-reutilizado)
2. [Compilación en ZinjaI](#2-compilación-en-zinjai)
3. [Controles](#3-controles)
4. [Estructura de cada polígono y su color](#4-estructura-de-cada-polígono-y-su-color)
5. [Pruebas realizadas](#5-pruebas-realizadas)
6. [Archivos del repositorio](#6-archivos-del-repositorio)

## 1. Código reutilizado

| Actividad anterior | Qué se reutilizó |
|---|---|
| `LineasMouse.cpp` / `Poligono.cpp` | Algoritmo de **Bresenham**, captura de vértices con el mouse y línea guía hasta el cursor |
| `variospoligonos.cpp` | Lista de **varios polígonos**, cierre con clic derecho y selección del polígono activo |
| `transformaciones.cpp` | **Matrices homogéneas 3×3**: identidad, multiplicación, traslación, rotación y escala respecto al centro |
| `Relleno.cpp` | Relleno **Scan-Line con ET y EAT** y selección de color con `r`, `g`, `b` |

## 2. Compilación en ZinjaI

**Opción A (recomendada).** Abrir el proyecto [`IntegracionFinal.zpr`](IntegracionFinal.zpr) desde *Archivo → Abrir* y presionar **F9**. El proyecto ya trae configurado:

- Directorios: `${MINGW_DIR}\OpenGl\include` y `${MINGW_DIR}\OpenGl\lib`
- Macro: `FREEGLUT_STATIC`
- Bibliotecas: `freeglut_static, glu32, opengl32, winmm, gdi32`

**Opción B.** Abrir [`IntegracionFinal.cpp`](IntegracionFinal.cpp) como programa simple. Luego, en *Ejecución → Opciones…*, agregar en *Parámetros extra para el compilador*:

```text
-DFREEGLUT_STATIC -lfreeglut_static -lglu32 -lopengl32 -lwinmm -lgdi32 -L${MINGW_DIR}\OpenGl\lib -I${MINGW_DIR}\OpenGl\include
```

Son las mismas opciones de la plantilla *"OpenGL - Windows"* de ZinjaI.

> **Nota:** el ejecutable y los archivos intermedios (`debug.w32/`, `release.w32/`, `*.o`, `*.exe`) no se suben al repositorio (ver [`.gitignore`](.gitignore)). Se generan al compilar.

## 3. Controles

| Control | Acción |
|---|---|
| Clic izquierdo | Agregar vértice |
| Clic derecho | Cerrar polígono (mínimo 3 vértices) |
| `1` … `9` | Seleccionar polígono activo |
| `D` / `I` | Rotar 5° a la derecha / izquierda |
| `S` / `s` | Aumentar / disminuir la escala un 10 % |
| Flechas | Trasladar el polígono activo 10 píxeles |
| `r`, `g`, `b` | Seleccionar color de relleno (rojo, verde, azul) |
| `P` | Rellenar el polígono activo con el color seleccionado |
| `C` | Limpiar todo |
| `ESC` | Salir |

El polígono activo se dibuja en **naranja** y los demás en negro. Arriba a la izquierda se muestran el polígono activo, su color y el color de relleno seleccionado. La consola muestra cada vértice agregado y la **matriz homogénea compuesta** de cada transformación.

## 4. Estructura de cada polígono y su color

```cpp
struct Punto
{
    float x;
    float y;
};

struct Color
{
    float r, g, b;
};

struct Poligono
{
    vector<Punto> P;   // vértices en orden
    bool cerrado;      // true cuando se cerró con clic derecho
    bool relleno;      // true cuando se rellenó con la tecla P
    Color color;       // color de relleno propio de este polígono
};

vector<Poligono> poligonos;  // todos los polígonos de la escena
int poligonoActual;          // polígono que se está construyendo con el mouse
int poligonoActivo;          // polígono seleccionado (1..9)
Color colorSeleccionado;     // color elegido con r, g, b
```

- **Vértices en `float`.** Las rotaciones y escalas se aplican una sobre otra. Si los vértices se guardaran como enteros, el error de redondeo se acumularía y el polígono se deformaría. Por eso se guardan en `float` y solo se redondean a enteros al rasterizar (Bresenham y ET).
- **Color guardado en cada polígono.** Las teclas `r`, `g`, `b` solo cambian `colorSeleccionado`. Al presionar `P`, ese color se **copia** dentro del polígono activo (`pol.color = colorSeleccionado`) y se marca `pol.relleno = true`. Así cada polígono conserva su propio color aunque después se elija otro o se cambie de polígono activo.
- **Relleno recalculado en cada cuadro.** En cada redibujado se reconstruye la ET a partir de los vértices actuales. Por eso el relleno sigue correctamente al polígono después de rotarlo, escalarlo o trasladarlo.
- **Transformaciones con matrices homogéneas.** Todas usan matrices 3×3:
  - Rotación: `M = T(C) · R(θ) · T(−C)`
  - Escala: `M = T(C) · S(s) · T(−C)`
  - Traslación: `M = T(tx, ty)`

  `C` es el centro del polígono, así el polígono gira y se escala sobre sí mismo. Cada vértice se transforma como `P' = M · [x y 1]ᵀ`.
- **Relleno Scan-Line.** Se construye la ET con un "cesto" por cada `y`, indexada con `y − yMin` para que funcione aunque el polígono salga de la ventana. Las aristas horizontales no ingresan. En cada línea de barrido:
  1. Se pasan a la EAT las aristas con `ymin = y`.
  2. Se retiran las que tienen `ymax = y`.
  3. Se ordena por `x`.
  4. Se pintan los píxeles entre pares de intersecciones, uno por uno con `GL_POINTS`.
  5. Se actualiza `x += 1/m`.

  **No se usa `GL_POLYGON`** ni ninguna otra primitiva de relleno.

## 5. Pruebas realizadas

| # | Prueba requerida | Evidencia |
|---|---|---|
| 1 | Polígono convexo | [04_rotacion_relleno_convexo.png](capturas/04_rotacion_relleno_convexo.png) |
| 2 | Polígono cóncavo | [06_escalamiento_relleno_concavo.png](capturas/06_escalamiento_relleno_concavo.png) |
| 3 | Al menos dos polígonos simultáneamente | [02_dos_poligonos_bresenham.png](capturas/02_dos_poligonos_bresenham.png) |
| 4 | Dos polígonos con colores de relleno diferentes | [08_traslacion_relleno.png](capturas/08_traslacion_relleno.png) |
| 5 | Rotación seguida de relleno | [03_rotacion.png](capturas/03_rotacion.png) → [04_rotacion_relleno_convexo.png](capturas/04_rotacion_relleno_convexo.png) |
| 6 | Escalamiento seguido de relleno | [05_escalamiento.png](capturas/05_escalamiento.png) → [06_escalamiento_relleno_concavo.png](capturas/06_escalamiento_relleno_concavo.png) |
| 7 | Traslación seguida de relleno | [07_traslacion.png](capturas/07_traslacion.png) → [08_traslacion_relleno.png](capturas/08_traslacion_relleno.png) |

La salida de la consola de esta ejecución (vértices y matrices de cada transformación) está en [`capturas/salida_consola.txt`](capturas/salida_consola.txt).

### Creación de polígonos con el mouse

El polígono 1 ya está cerrado y el polígono 2 está en construcción, con la línea guía (gris) hasta el cursor.

![Creación con el mouse](capturas/01_creacion_con_mouse.png)

### Dos polígonos simultáneos

Ambos polígonos están cerrados y su contorno está rasterizado con Bresenham. El activo (2) se ve en naranja.

![Dos polígonos](capturas/02_dos_poligonos_bresenham.png)

### Rotación seguida de relleno (polígono convexo)

Se presiona `1`, luego `D` seis veces (−30°):

![Rotación](capturas/03_rotacion.png)

Luego se presiona `r` y `P`:

![Rotación y relleno](capturas/04_rotacion_relleno_convexo.png)

### Escalamiento seguido de relleno (polígono cóncavo)

Se presiona `2`, luego `S` dos veces (×1.21):

![Escalamiento](capturas/05_escalamiento.png)

Luego se presiona `g` y `P`:

![Escalamiento y relleno](capturas/06_escalamiento_relleno_concavo.png)

### Traslación seguida de relleno

Se crea el polígono 3:

![Polígono 3 creado](capturas/07a_poligono3_creado.png)

Se traslada con las flechas (↑ ×5, ← ×3):

![Traslación](capturas/07_traslacion.png)

Luego se presiona `b` y `P`. Quedan tres polígonos con tres colores distintos:

![Traslación y relleno](capturas/08_traslacion_relleno.png)

### El color se conserva

Se vuelve a seleccionar el polígono 1 y se rota, traslada y reduce. Sigue rojo aunque el color seleccionado ahora es azul, y los polígonos 2 y 3 mantienen su color.

![Color conservado](capturas/09_color_conservado.png)

## 6. Archivos del repositorio

| Archivo | Descripción |
|---|---|
| [`IntegracionFinal.cpp`](IntegracionFinal.cpp) | Código fuente de la aplicación integrada |
| [`IntegracionFinal.zpr`](IntegracionFinal.zpr) | Proyecto de ZinjaI ya configurado para OpenGL |
| [`capturas/`](capturas) | Capturas de las pruebas y salida de la consola |
| [`.gitignore`](.gitignore) | Excluye los archivos generados al compilar |

## Preguntas de Rasterización de polígonos y transformaciones geométricas.

# Preguntas y respuestas: Rasterización de polígonos y transformaciones geométricas

## 1. ¿Qué diferencia existe entre rasterizar la frontera de un polígono y rellenar su interior?

La rasterización de la frontera de un polígono consiste en determinar qué píxeles deben dibujarse para representar sus aristas o bordes. Para ello, se utilizan algoritmos que calculan los píxeles que mejor aproximan las líneas que conectan los vértices del polígono en la pantalla.

Por otro lado, el relleno del interior consiste en identificar los píxeles que se encuentran dentro de la frontera del polígono para colorearlos. En el algoritmo de relleno mediante líneas de barrido (*Scanline*), se recorren las filas de la imagen y se calculan las intersecciones de cada fila con las aristas del polígono. Luego, se rellenan los píxeles comprendidos entre pares de intersecciones.

Por ejemplo, si tenemos un polígono cuadrado, la rasterización de la frontera dibuja únicamente sus cuatro lados, mientras que el relleno colorea toda la región comprendida entre ellos.

En conclusión, **la frontera define el contorno del polígono y el relleno representa su superficie interior**. Ambos procesos son complementarios, pero cumplen funciones diferentes dentro de la representación gráfica.

## 2. ¿Por qué las aristas horizontales no se incorporan a la ET en el algoritmo trabajado?

Las aristas horizontales no se incorporan a la Tabla de Aristas (ET, *Edge Table*) porque sus coordenadas inicial y final tienen el mismo valor en el eje Y. Por lo tanto, la variación vertical es cero.

La variación vertical se calcula mediante la siguiente expresión:

$$
\Delta y = y_{\max} - y_{\min}
$$

En una arista horizontal:

$$
y_{\max} = y_{\min}
$$

Por lo tanto:

$$
\Delta y = 0
$$

El algoritmo de relleno mediante líneas de barrido utiliza la pendiente inversa de cada arista para calcular cómo cambia la coordenada X cuando aumenta la coordenada Y:

$$
m^{-1} = \frac{\Delta x}{\Delta y}
$$

Si se incorpora una arista horizontal, se produciría una división entre cero, lo cual no está definido matemáticamente.

Además, una arista horizontal no atraviesa diferentes filas de barrido, ya que todos sus puntos tienen la misma coordenada Y. Por ello, no es necesario incluirla en la ET para calcular las intersecciones utilizadas en el relleno interior.

**En conclusión**, se excluyen las aristas horizontales porque no aportan intersecciones entre distintas filas de barrido y porque su inclusión provocaría problemas al calcular la pendiente inversa.

## 3. ¿Por qué una arista deja de estar activa al alcanzar su ymax?

Una arista deja de estar activa cuando la línea de barrido alcanza su coordenada máxima en Y (\(y_{\max}\)), porque a partir de ese punto ya no debe participar en el cálculo de las intersecciones de las siguientes filas.

En el algoritmo, cada arista se registra en la ET con información como su coordenada mínima en Y, su coordenada máxima en Y, la coordenada X inicial de la intersección y su pendiente inversa. Cuando la línea de barrido llega al extremo superior de una arista, esta debe retirarse de la Tabla de Aristas Activas (EAT, *Active Edge Table*).

La condición de eliminación se expresa habitualmente de la siguiente manera:

$$
y \geq y_{\max}
$$

Donde:

- \(y\): coordenada actual de la línea de barrido.
- \(y_{\max}\): coordenada vertical máxima de la arista.

Esta condición permite trabajar con un intervalo vertical que incluye el extremo inferior, pero excluye el extremo superior. De esta manera, se evita contar dos veces los vértices compartidos por dos aristas consecutivas, especialmente cuando una línea de barrido pasa exactamente por un vértice.

**En conclusión**, eliminar una arista al alcanzar su \(y_{\max}\) permite mantener actualizada la EAT, evitar intersecciones duplicadas y realizar el relleno del polígono de manera consistente.

## 4. ¿Qué representa Δx/Δy durante la actualización de la EAT?

El valor Δx/Δy representa la pendiente inversa de una arista, es decir, cuánto cambia la coordenada horizontal X por cada unidad que avanza la coordenada vertical Y.
Durante el algoritmo de relleno mediante líneas de barrido, este valor permite actualizar la posición de la intersección de una arista con cada nueva fila de la imagen. En lugar de calcular nuevamente la ecuación completa de la recta en cada iteración, el algoritmo utiliza el valor previamente calculado para determinar cuánto debe desplazarse horizontalmente la intersección.
Por ejemplo, si una arista se desplaza 6 unidades horizontalmente mientras sube 3 unidades verticalmente, su pendiente inversa indica que la coordenada X aumenta 2 unidades por cada fila que avanza la línea de barrido.
En conclusión, este valor permite actualizar de manera eficiente las intersecciones almacenadas en la Tabla de Aristas Activas (EAT), reduciendo los cálculos necesarios y facilitando el relleno correcto del interior del polígono.

## 5. ¿Por qué ET y EAT deben calcularse a partir de las coordenadas actuales después de transformar un polígono?

Las transformaciones geométricas, como la traslación, la rotación y el escalamiento, modifican las coordenadas de los vértices del polígono. Estos cambios pueden alterar la posición, la orientación, el tamaño y las pendientes de sus aristas.

Por este motivo, la Tabla de Aristas (ET) debe construirse utilizando las coordenadas actualizadas después de aplicar las transformaciones. A partir de esta tabla, se organiza la información necesaria para inicializar y actualizar la Tabla de Aristas Activas (EAT) durante el relleno.

Por ejemplo, la rotación de un polígono puede modificar las pendientes de sus aristas, mientras que la traslación cambia sus coordenadas mínimas y máximas. El escalamiento, por su parte, puede modificar tanto sus dimensiones como las coordenadas de intersección.

Si se utilizaran las tablas calculadas antes de la transformación, el algoritmo trabajaría con datos que ya no corresponden a la geometría actual del polígono. Esto podría producir un relleno incorrecto, intersecciones fuera de lugar o aristas activas en filas que no corresponden.

El procedimiento correcto es:

1. Obtener las coordenadas originales de los vértices.
2. Aplicar las transformaciones geométricas necesarias.
3. Actualizar las coordenadas de todos los vértices.
4. Construir nuevamente la ET con las coordenadas transformadas.
5. Inicializar y actualizar la EAT durante el proceso de relleno.

**En conclusión**, las tablas ET y EAT deben reflejar siempre la geometría actual del polígono para garantizar que las intersecciones y el relleno correspondan a su posición, tamaño y orientación finales.

## 6. ¿Qué ventaja ofrecen las coordenadas homogéneas para integrar traslación, rotación y escalamiento?

Las coordenadas homogéneas permiten representar las transformaciones geométricas mediante matrices de tamaño \(3 \times 3\) en dos dimensiones. Su principal ventaja es que permiten expresar la traslación, la rotación y el escalamiento utilizando una misma estructura matemática.

Un punto bidimensional se representa mediante tres componentes:

$$
P =
\begin{bmatrix}
x \\
y \\
1
\end{bmatrix}
$$

### Traslación

La traslación permite desplazar un punto una distancia \(t_x\) en el eje X y una distancia \(t_y\) en el eje Y. Su matriz es:

$$
T =
\begin{bmatrix}
1 & 0 & t_x \\
0 & 1 & t_y \\
0 & 0 & 1
\end{bmatrix}
$$

Al multiplicar la matriz por el punto, se obtiene:

$$
P' = TP =
\begin{bmatrix}
x+t_x \\
y+t_y \\
1
\end{bmatrix}
$$

### Rotación

La rotación permite girar un punto un ángulo \(\theta\) alrededor del origen. Su matriz es:

$$
R =
\begin{bmatrix}
\cos\theta & -\sin\theta & 0 \\
\sin\theta & \cos\theta & 0 \\
0 & 0 & 1
\end{bmatrix}
$$

### Escalamiento

El escalamiento permite modificar el tamaño de un objeto mediante los factores \(s_x\) y \(s_y\). Su matriz es:

$$
S =
\begin{bmatrix}
s_x & 0 & 0 \\
0 & s_y & 0 \\
0 & 0 & 1
\end{bmatrix}
$$

### Combinación de transformaciones

Una de las ventajas más importantes de las coordenadas homogéneas es que permiten combinar varias transformaciones en una sola matriz compuesta. Por ejemplo, para aplicar primero el escalamiento, después la rotación y finalmente la traslación, se utiliza:

$$
P' = T R S P
$$

En este caso, las matrices se multiplican en el orden indicado y la matriz resultante permite transformar el punto mediante una sola multiplicación.

Esto simplifica la implementación del programa, evita escribir procedimientos independientes para cada combinación de transformaciones y facilita aplicar las mismas operaciones a todos los vértices de un polígono.

**En conclusión**, las coordenadas homogéneas unifican las transformaciones geométricas en una representación matricial común, facilitan su combinación y permiten implementar el movimiento, la rotación y el cambio de tamaño de los polígonos de forma más organizada y eficiente.
