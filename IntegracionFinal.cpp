// =====================================================================
//  Computacion Grafica - Actividad 6: Integracion final
//
//  Aplicacion que reune lo desarrollado en las actividades anteriores:
//    - LineasMouse.cpp / Poligono.cpp   -> Bresenham y captura con mouse
//    - variospoligonos.cpp              -> varios poligonos, poligono activo
//    - transformaciones.cpp             -> matrices homogeneas 3x3
//    - Relleno.cpp                      -> relleno Scan-Line con ET y EAT
//
//  Controles:
//    Clic izquierdo   Agregar vertice al poligono en construccion
//    Clic derecho     Cerrar poligono
//    1 .. 9           Seleccionar poligono activo
//    D / I            Rotar 5 grados a la derecha / izquierda
//    S / s            Aumentar / disminuir escala (10%)
//    Flechas          Trasladar el poligono activo (10 pixeles)
//    r, g, b          Seleccionar color de relleno (rojo, verde, azul)
//    P                Rellenar el poligono activo con el color seleccionado
//    C                Limpiar todo
//    ESC              Salir
//
//  Restricciones cumplidas:
//    - Rotacion, escala y traslacion con matrices homogeneas 3x3.
//    - El contorno se rasteriza con Bresenham (GL_POINTS).
//    - El relleno usa Scan-Line con ET y EAT, pixel por pixel (GL_POINTS).
//      No se usa GL_POLYGON ni ninguna otra primitiva de relleno.
//
//  Compilar en ZinjaI: abrir IntegracionFinal.zpr (proyecto ya configurado)
//  o, como programa simple, en Ejecucion -> Opciones agregar:
//    -DFREEGLUT_STATIC -lfreeglut_static -lglu32 -lopengl32 -lwinmm -lgdi32
//    -L${MINGW_DIR}\OpenGl\lib -I${MINGW_DIR}\OpenGl\include
// =====================================================================

#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cmath>

using namespace std;

const int ANCHO = 800;
const int ALTO  = 600;
const float PI = 3.14159265f;

const float ANGULO_ROTACION = 5.0f;    // grados por pulsacion
const float FACTOR_AUMENTO  = 1.10f;   // S
const float FACTOR_REDUCIR  = 0.90f;   // s
const float PASO_TRASLACION = 10.0f;   // pixeles por pulsacion


// ---------------------------------------------------------
// ESTRUCTURAS
// ---------------------------------------------------------

// Los vertices se guardan en float para que las transformaciones
// sucesivas no acumulen error de redondeo. Al rasterizar se
// redondean a enteros.
struct Punto
{
	float x;
	float y;
};

struct Color
{
	float r;
	float g;
	float b;
};

// Cada poligono guarda sus vertices, si esta cerrado, si esta
// relleno y el color de relleno que se le asigno con la tecla P.
struct Poligono
{
	vector<Punto> P;
	bool cerrado;
	bool relleno;
	Color color;

	Poligono()
	{
		cerrado = false;
		relleno = false;
		color.r = 1.0f;
		color.g = 0.0f;
		color.b = 0.0f;
	}
};

// Arista de la tabla de aristas (ET) y de la tabla de aristas activas (EAT)
struct Arista
{
	int ymin;
	int ymax;
	float x;      // interseccion actual con la linea de barrido
	float invM;   // 1/m = dx/dy
};


// ---------------------------------------------------------
// ESTADO GLOBAL
// ---------------------------------------------------------

vector<Poligono> poligonos;   // el ultimo siempre es el que se esta construyendo
int poligonoActual = 0;       // indice del poligono en construccion
int poligonoActivo = -1;      // indice del poligono seleccionado (-1 = ninguno)
Punto Pmouse;

Color colorSeleccionado = {1.0f, 0.0f, 0.0f};   // color elegido con r, g, b


// ---------------------------------------------------------
// UTILIDADES
// ---------------------------------------------------------

int redondear(float v)
{
	return (int)floor(v + 0.5f);
}

// Pinta el pixel (x, y). Se usa el centro del pixel para que el
// punto caiga exactamente dentro de el.
void pintarPixel(int x, int y)
{
	glVertex2f(x + 0.5f, y + 0.5f);
}

const char* nombreColor(Color c)
{
	if (c.r == 1.0f && c.g == 0.0f && c.b == 0.0f) return "rojo";
	if (c.r == 0.0f && c.g == 1.0f && c.b == 0.0f) return "verde";
	if (c.r == 0.0f && c.g == 0.0f && c.b == 1.0f) return "azul";
	return "personalizado";
}

bool hayPoligonoActivo()
{
	return poligonoActivo >= 0 &&
		   poligonoActivo < (int)poligonos.size() &&
		   poligonos[poligonoActivo].cerrado;
}

int numeroPoligonosCerrados()
{
	int n = 0;
	for (int i = 0; i < (int)poligonos.size(); i++)
		if (poligonos[i].cerrado)
			n++;
	return n;
}


// ---------------------------------------------------------
// ALGORITMO DE BRESENHAM (de LineasMouse.cpp / Poligono.cpp)
// Funciona para cualquier pendiente
// ---------------------------------------------------------

void Bresenham(int x1, int y1, int x2, int y2)
{
	int dx = abs(x2 - x1);
	int dy = abs(y2 - y1);

	// Direccion de avance en X y en Y
	int sx = (x1 < x2) ? 1 : -1;
	int sy = (y1 < y2) ? 1 : -1;

	// Parametro de decision
	int p = dx - dy;

	while (true)
	{
		pintarPixel(x1, y1);

		if (x1 == x2 && y1 == y2)
			break;

		int p2 = 2 * p;

		if (p2 > -dy)
		{
			p = p - dy;
			x1 = x1 + sx;
		}

		if (p2 < dx)
		{
			p = p + dx;
			y1 = y1 + sy;
		}
	}
}


// ---------------------------------------------------------
// MATRICES HOMOGENEAS 3x3 (de transformaciones.cpp)
// ---------------------------------------------------------

void identidad(float M[3][3])
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			M[i][j] = (i == j) ? 1.0f : 0.0f;
}

// C = A * B
void multiplicar(float A[3][3], float B[3][3], float C[3][3])
{
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
		{
			C[i][j] = 0;
			for (int k = 0; k < 3; k++)
				C[i][j] += A[i][k] * B[k][j];
		}
}

//     | 1  0  tx |
// T = | 0  1  ty |
//     | 0  0  1  |
void matrizTraslacion(float tx, float ty, float T[3][3])
{
	identidad(T);
	T[0][2] = tx;
	T[1][2] = ty;
}

//     | cos  -sin  0 |
// R = | sin   cos  0 |
//     |  0     0   1 |
void matrizRotacion(float angulo, float R[3][3])
{
	identidad(R);
	float rad = angulo * PI / 180.0f;
	R[0][0] = cos(rad);
	R[0][1] = -sin(rad);
	R[1][0] = sin(rad);
	R[1][1] = cos(rad);
}

//     | sx  0   0 |
// S = | 0   sy  0 |
//     | 0   0   1 |
void matrizEscala(float sx, float sy, float S[3][3])
{
	identidad(S);
	S[0][0] = sx;
	S[1][1] = sy;
}

// P' = M * [x y 1]^T
Punto transformarPunto(Punto P, float M[3][3])
{
	Punto P2;
	P2.x = M[0][0] * P.x + M[0][1] * P.y + M[0][2];
	P2.y = M[1][0] * P.x + M[1][1] * P.y + M[1][2];
	return P2;
}

void transformar(Poligono &pol, float M[3][3])
{
	for (int i = 0; i < (int)pol.P.size(); i++)
		pol.P[i] = transformarPunto(pol.P[i], M);
}

void imprimirMatriz(const char *nombre, float M[3][3])
{
	cout << nombre << " =" << endl;
	for (int i = 0; i < 3; i++)
	{
		char linea[80];
		sprintf(linea, "   | %9.4f %9.4f %9.4f |", M[i][0], M[i][1], M[i][2]);
		cout << linea << endl;
	}
}

Punto centroPoligono(Poligono &pol)
{
	Punto C = {0, 0};
	for (int i = 0; i < (int)pol.P.size(); i++)
	{
		C.x += pol.P[i].x;
		C.y += pol.P[i].y;
	}
	C.x /= pol.P.size();
	C.y /= pol.P.size();
	return C;
}

// Rotacion respecto al centro: M = T(C) * R(angulo) * T(-C)
void rotar(Poligono &pol, float angulo)
{
	Punto C = centroPoligono(pol);
	float T1[3][3], R[3][3], T2[3][3], M1[3][3], M[3][3];

	matrizTraslacion(-C.x, -C.y, T1);
	matrizRotacion(angulo, R);
	matrizTraslacion(C.x, C.y, T2);

	multiplicar(R, T1, M1);
	multiplicar(T2, M1, M);

	transformar(pol, M);

	cout << endl << "Rotacion de " << angulo << " grados respecto al centro ("
		 << C.x << ", " << C.y << ")" << endl;
	imprimirMatriz("M = T(C) * R * T(-C)", M);
}

// Escalamiento respecto al centro: M = T(C) * S(factor) * T(-C)
void escalar(Poligono &pol, float factor)
{
	Punto C = centroPoligono(pol);
	float T1[3][3], S[3][3], T2[3][3], M1[3][3], M[3][3];

	matrizTraslacion(-C.x, -C.y, T1);
	matrizEscala(factor, factor, S);
	matrizTraslacion(C.x, C.y, T2);

	multiplicar(S, T1, M1);
	multiplicar(T2, M1, M);

	transformar(pol, M);

	cout << endl << "Escalamiento por " << factor << " respecto al centro ("
		 << C.x << ", " << C.y << ")" << endl;
	imprimirMatriz("M = T(C) * S * T(-C)", M);
}

// Traslacion: M = T(tx, ty)
void trasladar(Poligono &pol, float tx, float ty)
{
	float T[3][3];
	matrizTraslacion(tx, ty, T);
	transformar(pol, T);

	cout << endl << "Traslacion (" << tx << ", " << ty << ")" << endl;
	imprimirMatriz("T", T);
}


// ---------------------------------------------------------
// RELLENO SCAN-LINE CON ET Y EAT (de Relleno.cpp)
// ---------------------------------------------------------

// Construye la tabla de aristas. Como el poligono puede estar en
// cualquier posicion (incluso fuera de la ventana despues de una
// traslacion), la ET se indexa con y - yMin.
vector< vector<Arista> > construirET(const vector<Punto> &V, int yMin, int yMax)
{
	vector< vector<Arista> > ET(yMax - yMin + 1);
	int n = V.size();

	for (int i = 0; i < n; i++)
	{
		int x1 = redondear(V[i].x);
		int y1 = redondear(V[i].y);
		int x2 = redondear(V[(i + 1) % n].x);
		int y2 = redondear(V[(i + 1) % n].y);

		// Las aristas horizontales no ingresan a la ET
		if (y1 == y2)
			continue;

		Arista A;
		if (y1 < y2)
		{
			A.ymin = y1;
			A.ymax = y2;
			A.x = (float)x1;
		}
		else
		{
			A.ymin = y2;
			A.ymax = y1;
			A.x = (float)x2;
		}
		A.invM = (float)(x2 - x1) / (float)(y2 - y1);

		ET[A.ymin - yMin].push_back(A);
	}
	return ET;
}

bool compararX(const Arista &A, const Arista &B)
{
	return A.x < B.x;
}

bool aristaTerminada(const Arista &A, int y)
{
	return A.ymax == y;
}

void rellenarScanLine(const Poligono &pol)
{
	const vector<Punto> &V = pol.P;
	if (V.size() < 3)
		return;

	// 1. Rango vertical del poligono
	int yMin = redondear(V[0].y);
	int yMax = yMin;
	for (int i = 1; i < (int)V.size(); i++)
	{
		yMin = min(yMin, redondear(V[i].y));
		yMax = max(yMax, redondear(V[i].y));
	}

	vector< vector<Arista> > ET = construirET(V, yMin, yMax);

	// 2. La EAT empieza vacia
	vector<Arista> EAT;

	glColor3f(pol.color.r, pol.color.g, pol.color.b);
	glBegin(GL_POINTS);

	// 3. Recorrer las lineas de barrido
	for (int y = yMin; y < yMax; y++)
	{
		// 3.1 Pasar de la ET a la EAT las aristas cuyo ymin = y
		vector<Arista> &cesto = ET[y - yMin];
		for (int i = 0; i < (int)cesto.size(); i++)
			EAT.push_back(cesto[i]);

		// 3.2 Retirar las aristas cuyo ymax = y
		//     (el vertice ymax no se cuenta en la paridad)
		vector<Arista> quedan;
		for (int i = 0; i < (int)EAT.size(); i++)
			if (!aristaTerminada(EAT[i], y))
				quedan.push_back(EAT[i]);
		EAT = quedan;

		// Mantener la EAT ordenada en x
		sort(EAT.begin(), EAT.end(), compararX);

		// 3.3 Pintar los pixeles entre pares de intersecciones
		//     (regla de paridad par-impar). Solo dentro de la ventana.
		if (y >= 0 && y < ALTO)
		{
			for (int i = 0; i + 1 < (int)EAT.size(); i += 2)
			{
				int xIni = (int)ceil(EAT[i].x);
				int xFin = (int)floor(EAT[i + 1].x);

				if (xIni < 0) xIni = 0;
				if (xFin > ANCHO - 1) xFin = ANCHO - 1;

				for (int x = xIni; x <= xFin; x++)
					pintarPixel(x, y);
			}
		}

		// 3.4 / 3.5 Actualizar x para la siguiente linea: x = x + 1/m
		for (int i = 0; i < (int)EAT.size(); i++)
			EAT[i].x += EAT[i].invM;
	}

	glEnd();
}


// ---------------------------------------------------------
// DIBUJO
// ---------------------------------------------------------

// Contorno con Bresenham. Si el poligono no esta cerrado se dibuja
// abierto y, si es el que se esta construyendo, con la linea guia
// hasta el mouse.
void dibujarContorno(Poligono &pol, bool esActivo, bool enConstruccion)
{
	int n = pol.P.size();
	if (n == 0)
		return;

	if (esActivo)
	{
		glColor3f(1.0f, 0.5f, 0.0f);   // naranja: poligono activo
		glPointSize(2.0f);
	}
	else
	{
		glColor3f(0.0f, 0.0f, 0.0f);   // negro: resto de poligonos
		glPointSize(1.0f);
	}

	glBegin(GL_POINTS);
	for (int i = 0; i < n - 1; i++)
		Bresenham(redondear(pol.P[i].x), redondear(pol.P[i].y),
				  redondear(pol.P[i + 1].x), redondear(pol.P[i + 1].y));

	if (pol.cerrado && n >= 3)
		Bresenham(redondear(pol.P[n - 1].x), redondear(pol.P[n - 1].y),
				  redondear(pol.P[0].x), redondear(pol.P[0].y));
	glEnd();
	glPointSize(1.0f);

	// Linea guia (gris) desde el ultimo vertice hasta el mouse
	if (enConstruccion && !pol.cerrado)
	{
		glColor3f(0.5f, 0.5f, 0.5f);
		glBegin(GL_POINTS);
		Bresenham(redondear(pol.P[n - 1].x), redondear(pol.P[n - 1].y),
				  redondear(Pmouse.x), redondear(Pmouse.y));
		glEnd();
	}

	// Marcar los vertices
	glPointSize(5.0f);
	if (esActivo)
		glColor3f(1.0f, 0.5f, 0.0f);
	else
		glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_POINTS);
	for (int i = 0; i < n; i++)
		pintarPixel(redondear(pol.P[i].x), redondear(pol.P[i].y));
	glEnd();
	glPointSize(1.0f);
}

void escribirTexto(int x, int y, const char *texto)
{
	glRasterPos2i(x, y);
	for (const char *c = texto; *c; c++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
}

// Numero de cada poligono junto a su primer vertice
void dibujarEtiquetas()
{
	for (int i = 0; i < (int)poligonos.size(); i++)
	{
		if (!poligonos[i].cerrado || poligonos[i].P.empty())
			continue;
		char etiqueta[8];
		sprintf(etiqueta, "%d", i + 1);
		if (i == poligonoActivo)
			glColor3f(0.8f, 0.3f, 0.0f);
		else
			glColor3f(0.0f, 0.0f, 0.0f);
		Punto C = centroPoligono(poligonos[i]);
		escribirTexto(redondear(C.x) - 3, redondear(C.y) - 4, etiqueta);
	}
}

void dibujarInformacion()
{
	char linea[120];

	glColor3f(0.0f, 0.0f, 0.0f);
	if (hayPoligonoActivo())
	{
		Poligono &pol = poligonos[poligonoActivo];
		sprintf(linea, "Poligono activo: %d  (%d vertices, %s)",
				poligonoActivo + 1, (int)pol.P.size(),
				pol.relleno ? nombreColor(pol.color) : "sin relleno");
	}
	else
		sprintf(linea, "Poligono activo: ninguno");
	escribirTexto(10, ALTO - 20, linea);

	sprintf(linea, "Poligonos cerrados: %d", numeroPoligonosCerrados());
	escribirTexto(10, ALTO - 36, linea);

	// Muestra del color seleccionado
	escribirTexto(10, ALTO - 52, "Color de relleno:");
	glColor3f(colorSeleccionado.r, colorSeleccionado.g, colorSeleccionado.b);
	glPointSize(12.0f);
	glBegin(GL_POINTS);
	glVertex2i(122, ALTO - 48);
	glEnd();
	glPointSize(1.0f);

	glColor3f(0.35f, 0.35f, 0.35f);
	escribirTexto(10, 10,
				  "Clic izq: vertice | Clic der: cerrar | 1-9: activo | D/I: rotar | "
				  "S/s: escala | Flechas: mover | r g b: color | P: rellenar | C: limpiar");
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT);

	// Primero los rellenos (ET + EAT) y encima los contornos
	for (int i = 0; i < (int)poligonos.size(); i++)
		if (poligonos[i].cerrado && poligonos[i].relleno)
			rellenarScanLine(poligonos[i]);

	for (int i = 0; i < (int)poligonos.size(); i++)
		dibujarContorno(poligonos[i], i == poligonoActivo, i == poligonoActual);

	dibujarEtiquetas();
	dibujarInformacion();

	glutSwapBuffers();
}


// ---------------------------------------------------------
// EVENTOS
// ---------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
	if (state != GLUT_DOWN)
		return;

	y = ALTO - y;
	Poligono &pol = poligonos[poligonoActual];

	if (button == GLUT_LEFT_BUTTON)
	{
		Punto Pi;
		Pi.x = (float)x;
		Pi.y = (float)y;
		pol.P.push_back(Pi);

		cout << "Poligono " << poligonoActual + 1 << " - P" << pol.P.size()
			 << " = (" << x << ", " << y << ")" << endl;
		glutPostRedisplay();
	}

	if (button == GLUT_RIGHT_BUTTON)
	{
		if (pol.P.size() < 3)
		{
			cout << "Se necesitan al menos 3 vertices para cerrar el poligono." << endl;
			return;
		}

		pol.cerrado = true;
		cout << endl << "Poligono " << poligonoActual + 1 << " cerrado ("
			 << pol.P.size() << " vertices, " << pol.P.size() << " aristas)." << endl;

		// El poligono recien cerrado pasa a ser el activo
		poligonoActivo = poligonoActual;

		// Se prepara un poligono vacio para seguir dibujando
		poligonos.push_back(Poligono());
		poligonoActual = poligonos.size() - 1;

		cout << "Poligono activo: " << poligonoActivo + 1 << endl;
		glutPostRedisplay();
	}
}

void movimiento(int x, int y)
{
	Pmouse.x = (float)x;
	Pmouse.y = (float)(ALTO - y);
	glutPostRedisplay();
}

void seleccionarColor(float r, float g, float b)
{
	colorSeleccionado.r = r;
	colorSeleccionado.g = g;
	colorSeleccionado.b = b;
	cout << "Color de relleno seleccionado: " << nombreColor(colorSeleccionado) << endl;
}

void teclado(unsigned char tecla, int x, int y)
{
	// Seleccion de poligono activo: 1 .. 9
	if (tecla >= '1' && tecla <= '9')
	{
		int i = tecla - '1';
		if (i < (int)poligonos.size() && poligonos[i].cerrado)
		{
			poligonoActivo = i;
			cout << "Poligono " << i + 1 << " activo" << endl;
		}
		else
			cout << "El poligono " << i + 1 << " no existe o no esta cerrado." << endl;
	}

	// Color de relleno
	if (tecla == 'r') seleccionarColor(1.0f, 0.0f, 0.0f);
	if (tecla == 'g') seleccionarColor(0.0f, 1.0f, 0.0f);
	if (tecla == 'b') seleccionarColor(0.0f, 0.0f, 1.0f);

	if (hayPoligonoActivo())
	{
		Poligono &pol = poligonos[poligonoActivo];

		// D: rotar a la derecha (sentido horario)
		if (tecla == 'd' || tecla == 'D')
			rotar(pol, -ANGULO_ROTACION);

		// I: rotar a la izquierda (sentido antihorario)
		if (tecla == 'i' || tecla == 'I')
			rotar(pol, ANGULO_ROTACION);

		// S: aumentar escala / s: disminuir escala
		if (tecla == 'S')
			escalar(pol, FACTOR_AUMENTO);
		if (tecla == 's')
			escalar(pol, FACTOR_REDUCIR);

		// P: rellenar SOLO el poligono activo con el color seleccionado.
		// El color queda guardado en el poligono, asi que cambiar de
		// color despues no afecta a los poligonos ya rellenados.
		if (tecla == 'p' || tecla == 'P')
		{
			pol.relleno = true;
			pol.color = colorSeleccionado;
			cout << "Poligono " << poligonoActivo + 1 << " rellenado de "
				 << nombreColor(pol.color) << " (Scan-Line con ET y EAT)" << endl;
		}
	}

	if (tecla == 'c' || tecla == 'C')
	{
		poligonos.clear();
		poligonos.push_back(Poligono());
		poligonoActual = 0;
		poligonoActivo = -1;
		cout << endl << "Pantalla limpiada." << endl;
	}

	if (tecla == 27)
		exit(0);

	glutPostRedisplay();
}

// Flechas: trasladar el poligono activo
void teclasEspeciales(int tecla, int x, int y)
{
	if (!hayPoligonoActivo())
		return;

	Poligono &pol = poligonos[poligonoActivo];

	if (tecla == GLUT_KEY_LEFT)  trasladar(pol, -PASO_TRASLACION, 0);
	if (tecla == GLUT_KEY_RIGHT) trasladar(pol,  PASO_TRASLACION, 0);
	if (tecla == GLUT_KEY_UP)    trasladar(pol, 0,  PASO_TRASLACION);
	if (tecla == GLUT_KEY_DOWN)  trasladar(pol, 0, -PASO_TRASLACION);

	glutPostRedisplay();
}

// La ventana mantiene su tamano para que las coordenadas del mouse
// coincidan con las de la proyeccion
void reshape(int w, int h)
{
	if (w != ANCHO || h != ALTO)
		glutReshapeWindow(ANCHO, ALTO);
	glViewport(0, 0, ANCHO, ALTO);
}


// ---------------------------------------------------------

void inicializar()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, ANCHO, 0, ALTO);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	poligonos.push_back(Poligono());
	poligonoActual = 0;
	poligonoActivo = -1;
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(ANCHO, ALTO);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Integracion final - Poligonos, transformaciones y relleno");

	inicializar();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutPassiveMotionFunc(movimiento);
	glutKeyboardFunc(teclado);
	glutSpecialFunc(teclasEspeciales);

	cout << "Integracion final - Computacion Grafica" << endl;
	cout << "Dibuje un poligono con clic izquierdo y cierrelo con clic derecho." << endl;

	glutMainLoop();
	return 0;
}
