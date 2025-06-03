#include <Servo.h>
#include <Adafruit_LiquidCrystal.h>

// pantalla
Adafruit_LiquidCrystal pantalla_residuos(0);

const int sensor_pir = 9;
const int echoDistanciaContenedor = 11;
const int triggerDistanciaContenedor = 12;
const int motor_tapa = 10;

const int triggerContenido = 4;
const int echoContenido = 5;

int led_rojo = 3;
int led_verde = 2;
Servo servoCompuerta;

// variables y estados
int distanciaLimite = 150; // distancia limite en cm para abrir tapa
int limiteResiduos = 90;   // altura maxima para considerar contenedor lleno

bool espacioDisponible = true;
bool compuertaAbierta = false;
int conteoAperturas = 0;
int intentosFallidos = 0;

int auxiliarContenido = 0;

const char *mensajeSerial = "Contenedor lleno. Notificando a area de Mantenimientos.";

unsigned long tiempoPantallaEncendida = 0;
unsigned long duracionPantallaEncendida = 3000; // 5 segundos
bool pantallaEncendida = true;

//-----------------------------------------------------------------

void setup()
{
    Serial.begin(9600);
    pinMode(triggerContenido, OUTPUT);
    pinMode(echoContenido, INPUT);
    pinMode(led_verde, OUTPUT);
    pinMode(led_rojo, OUTPUT);

    pinMode(triggerDistanciaContenedor, OUTPUT);
    pinMode(echoDistanciaContenedor, INPUT);

    pinMode(sensor_pir, INPUT);

    pantalla_residuos.begin(16, 2);

    servoCompuerta.attach(motor_tapa);
    servoCompuerta.write(0);

    aptoRecibir(triggerContenido, echoContenido);
    apagarPantalla();
}

// -----------------------------------------------------------------
// manejador estados contenedor, pantalla y leds
// -----------------------------------------------------------------

void encenderPantalla()
{
    if (!pantallaEncendida)
    {
        pantalla_residuos.setBacklight(HIGH); // encendemos la pantalla
        pantallaEncendida = true;
    }
    tiempoPantallaEncendida = millis(); // reinicio el contaodr
}

void apagarPantalla()
{
    if (pantallaEncendida && millis() - tiempoPantallaEncendida >= duracionPantallaEncendida)
    {
        pantalla_residuos.setBacklight(LOW); // apagamos la pantalla
        pantallaEncendida = false;
    }
}

void estadoContenedor() // logica para estado inicial
{
    digitalWrite(led_verde, HIGH);
    digitalWrite(led_rojo, LOW);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Contenedor");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("Habilitado");
}
void habilitarContenedor() // logica al detectar cuerpo cerca
{
    digitalWrite(led_verde, HIGH);
    digitalWrite(led_rojo, LOW);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Ingresar");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("Residuos");
}

void deshabilitarContenedor() // logica por contenedor no disponible
{
    digitalWrite(led_verde, LOW);
    digitalWrite(led_rojo, HIGH);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Contenedor");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("Lleno");
}

void reciclajeExitoso() // logica confirmando reciclaje
{
    digitalWrite(led_verde, HIGH);
    digitalWrite(led_rojo, LOW);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Residuo recibido");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("Gracias!");
}

void contenedorAlMaximo() // logica por contenedor al maximo de capacidad
{
    digitalWrite(led_verde, LOW);
    digitalWrite(led_rojo, HIGH);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Cerrando al");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("maximo");
}

void mensajeMantenimiento() // logica por vaciado pendiente
{
    digitalWrite(led_verde, LOW);
    digitalWrite(led_rojo, HIGH);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Vaciado de box");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("pendiente");
}

void procesoEnRiesgo() // logica por contenedor al maximo de capacidad posterior al reciclaje
{
    digitalWrite(led_verde, LOW);
    digitalWrite(led_rojo, HIGH);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Registro de ");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("exceso");
    delay(1000);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    pantalla_residuos.print("Se cerrara");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("al tope.");
}

void abrirCompuerta() // abrir tapa contenedor
{
    servoCompuerta.write(90);
}

void cerrarCompuerta() // cerrar tapa contenedor
{
    servoCompuerta.write(0);
}

//-----------------------------------------------------------------
// Funcion re utilizable para medir distancias
//-----------------------------------------------------------------
long obtenerDistanciaSensor(int triggerPin, int echoPin)
{
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    long duracion = pulseIn(echoPin, HIGH);
    long distancia = duracion * 0.034 / 2;
    return distancia;
}
//-----------------------------------------------------------------
//-----------------------------------------------------------------

// devuelve posibilidad de recibir o no contenido
bool aptoRecibir(int triggerPin, int echoPin)
{
    if (obtenerDistanciaSensor(triggerPin, echoPin) >= limiteResiduos)
    {
        espacioDisponible = true;
        estadoContenedor();
        return true;
    }
    else
    {
        espacioDisponible = false;
        deshabilitarContenedor();
        return false;
    }
}

// validador de capacidad del contenedor
bool validarContenido(int triggerPin, int echoPin)
{
    if (obtenerDistanciaSensor(triggerPin, echoPin) >= limiteResiduos)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// validador de persona cerca
bool personaDetectada(int triggerPin, int echoPin)
{

    if (obtenerDistanciaSensor(triggerPin, echoPin) <= distanciaLimite)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void validadorEspacio()
{
    bool validacionEspacioContenedor = validarContenido(triggerContenido, echoContenido);

    if (!validacionEspacioContenedor && espacioDisponible)
    {
        deshabilitarContenedor();
        espacioDisponible = false;
    }
    else if (validacionEspacioContenedor && !espacioDisponible)
    {
        estadoContenedor();
        espacioDisponible = true;
    }
}

// ==================================================================
// ==================================================================
void loop()
{

    if (digitalRead(sensor_pir))
    {

        bool deteccionPersona = personaDetectada(triggerDistanciaContenedor, echoDistanciaContenedor);

        if (deteccionPersona)
        {
            encenderPantalla();
            validadorEspacio();

            if (espacioDisponible)
            {

                if (!compuertaAbierta)
                {
                    abrirCompuerta();
                    compuertaAbierta = true;
                    auxiliarContenido++;
                    delay(1000);
                    habilitarContenedor();
                }
            }
            else
            {
                if (auxiliarContenido > 0)
                {
                    procesoEnRiesgo();
                    delay(1000);
                    compuertaAbierta = true;
                }
                else
                {
                    mensajeMantenimiento();
                    intentosFallidos++;
                    Serial.println("\n=============================================");
                    Serial.println(mensajeSerial);
                    Serial.println("Se intento recilcar " + String(intentosFallidos) + " veces y no fue posible.");
                    delay(1000);
                    deshabilitarContenedor();
                }
            }
        }
        else
        {
            encenderPantalla();

            if (compuertaAbierta)
            {
                validadorEspacio();

                if (espacioDisponible)
                {
                    reciclajeExitoso();
                    compuertaAbierta = false;
                    conteoAperturas++;
                    Serial.println("\n=============================================");
                    Serial.println("Total aperturas: " + String(conteoAperturas));
                    delay(1000);
                    estadoContenedor();
                }
                else
                {
                    if (auxiliarContenido > 0)
                    {
                        compuertaAbierta = false;
                        conteoAperturas++;
                        delay(1000);
                        deshabilitarContenedor();
                    }
                    else
                    {
                        if (espacioDisponible)
                        {
                            reciclajeExitoso();
                            compuertaAbierta = false;
                            conteoAperturas++;
                            Serial.println("\n=============================================");
                            Serial.println("Total aperturas: " + String(conteoAperturas));
                            delay(1000);
                            estadoContenedor();
                        }
                        else
                        {
                            contenedorAlMaximo();
                            compuertaAbierta = false;
                            Serial.println("\n=============================================");
                            Serial.println(mensajeSerial);
                            delay(1000);
                            deshabilitarContenedor();
                        }
                    }
                }

                cerrarCompuerta();
            }
            auxiliarContenido = 0; // reset del auxiliar ante cambios en el contenido post reciclaje.
        }
    }
    apagarPantalla();
    delay(900);
}