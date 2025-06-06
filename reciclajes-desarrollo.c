// ================================================================================
// Librerias
#include <Servo.h>
#include <Adafruit_LiquidCrystal.h>

// ================================================================================
// Declaraciones
const int switch_deslizante = 8;   // interruptor manual
const int sensor_pri_princial = 9; // sensor de movimiento

const int echo_sensor_distancia = 12;    // echo del sensor de distancia
const int trigger_sensor_distancia = 13; // trigger del sensor de distancia

const int echo_sensor_contenido = 5;    // echo sensor de altura de residuos
const int trigger_sensor_contenido = 4; // trigger sensor de altura de residuos

// servo motor 1
const int servo_uno_compuerta = 10;
Servo instancia_servo_uno;

// servo motor 2
const int servo_dos_compuerta = 11;
Servo instancia_servo_dos;

Adafruit_LiquidCrystal pantalla_residuos(0); // pantalla LCD
int led_rojo = 3;  // led de color rojo
int led_verde = 2; // led de color verde

// ================================================================================
// variables y estados
int umbral_distancia_trigger = 150; // umbral de distancia minimo para detectar una persona cerca
int umbral_altura_contenedor = 90;  // umbral de altura maxima de residuos permitidos en el contenedor para declararlo como "disponible"

bool espacio_disponible = true; // variable para controlar el estado del contenedor
bool compuerta_abierta = false; // variable para controlar el estado de la compuerta

int delay_apertura_servo = 30; // variable para definir delay en apertura para evitar movimientos bruscos
int angulo_limite_servo = 60;  // limite de apertura de la compuerta

int conteo_aperturas_exitosos = 0; // variable para controlar el conteo de aperturas.
int conteo_aperturas_fallidos = 0; // variable para controlar el conteo de intentos fallidos

int variable_auxiliar_contenido = 0; // Se trata de una variable helper para ciertos casos hibridos

const char *mensaje_contendor_lleno = "Contenedor lleno. Notificando a area de Mantenimientos."; // mensaje de alerta re utilizado en varias ocasiones

unsigned long variable_auxiliar_tiempo_pantalla_encendida = 0; // variable para controlar el tiempo de encFendido de la pantalla
unsigned long duracion_pantalla_encendida = 3000; // Tiempo que la pantalla permanecera encendida
bool pantalla_encendida = true; // variable para controlar el estado de la pantalla
bool apertura_manual_contenedor = false; // variable para controlar el estado de la puerta

//-----------------------------------------------------------------

void setup()
{
    Serial.begin(9600);
    pinMode(trigger_sensor_contenido, OUTPUT);
    pinMode(echo_sensor_contenido, INPUT);
    pinMode(led_verde, OUTPUT);
    pinMode(led_rojo, OUTPUT);
    pinMode(switch_deslizante, INPUT);

    pinMode(trigger_sensor_distancia, OUTPUT);
    pinMode(echo_sensor_distancia, INPUT);

    pinMode(sensor_pri_princial, INPUT);

    pantalla_residuos.begin(16, 2);

    instancia_servo_uno.attach(servo_uno_compuerta);
    instancia_servo_uno.write(0);

    instancia_servo_dos.attach(servo_dos_compuerta);
    instancia_servo_dos.write(0);

    aptoRecibir(trigger_sensor_contenido, echo_sensor_contenido);
    apagarPantalla();
}

// -----------------------------------------------------------------
// manejador estados contenedor, pantalla y leds
// -----------------------------------------------------------------

void encenderPantalla()
{
    if (!pantalla_encendida)
    {
        pantalla_residuos.setBacklight(HIGH); // encendemos la pantalla
        pantalla_encendida = true;
    }
    variable_auxiliar_tiempo_pantalla_encendida = millis(); // reinicio el contaodr
}

void apagarPantalla()
{
    if (pantalla_encendida && millis() - variable_auxiliar_tiempo_pantalla_encendida >= duracion_pantalla_encendida)
    {
        pantalla_residuos.setBacklight(LOW); // apagamos la pantalla
        pantalla_encendida = false;
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

void puertaAbiertaManualmente() // logica para abrir puerta manualmente
{
    apertura_manual_contenedor = true;
    digitalWrite(led_verde, LOW);
    digitalWrite(led_rojo, HIGH);
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    encenderPantalla();
    pantalla_residuos.print("Apertura Manual");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("Puerta fija");
    if (!compuerta_abierta)
    {
        abrirCompuerta();
        compuerta_abierta = true;
    }

    apagarPantalla();
}

void puertaCerradaManualmente() // logica para cerrar puerta manualmente
{
    apertura_manual_contenedor = false;
    pantalla_residuos.clear();
    pantalla_residuos.setCursor(0, 0);
    encenderPantalla();
    pantalla_residuos.print("Cerrando");
    pantalla_residuos.setCursor(0, 1);
    pantalla_residuos.print("puerta normal");

    delay(2000);
    estadoContenedor();
    if (compuerta_abierta)
    {
        cerrarCompuerta();
        compuerta_abierta = false;
    }
}

void abrirCompuerta() // abrir tapa contenedor
{
    for (int pos = 0; pos <= angulo_limite_servo; pos++)
    {
        instancia_servo_uno.write(pos);
        instancia_servo_dos.write(pos);
        delay(delay_apertura_servo);
    }
}

void cerrarCompuerta() // cerrar tapa contenedor
{
    for (int pos = angulo_limite_servo; pos >= 0; pos--)
    {
        instancia_servo_uno.write(pos);
        instancia_servo_dos.write(pos);
        delay(delay_apertura_servo);
    }
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
    if (obtenerDistanciaSensor(triggerPin, echoPin) >= umbral_altura_contenedor)
    {
        espacio_disponible = true;
        estadoContenedor();
        return true;
    }
    else
    {
        espacio_disponible = false;
        deshabilitarContenedor();
        return false;
    }
}

// validador de capacidad del contenedor
bool validarContenido(int triggerPin, int echoPin)
{
    if (obtenerDistanciaSensor(triggerPin, echoPin) >= umbral_altura_contenedor)
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

    if (obtenerDistanciaSensor(triggerPin, echoPin) <= umbral_distancia_trigger)
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
    bool validacionEspacioContenedor = validarContenido(trigger_sensor_contenido, echo_sensor_contenido);

    if (!validacionEspacioContenedor && espacio_disponible)
    {
        deshabilitarContenedor();
        espacio_disponible = false;
    }
    else if (validacionEspacioContenedor && !espacio_disponible)
    {
        estadoContenedor();
        espacio_disponible = true;
    }
}

bool manejoManual()
{

    if (digitalRead(switch_deslizante) == HIGH && !apertura_manual_contenedor)
    {
        puertaAbiertaManualmente();
        return false;
    }
    else if (digitalRead(switch_deslizante) == LOW && apertura_manual_contenedor)
    {
        puertaCerradaManualmente();
        return false;
    }
    else
    {
        return true;
    }
}

// ==================================================================
// ==================================================================
void loop()
{

    bool validacionManual = manejoManual();
    if (digitalRead(sensor_pri_princial) && validacionManual)
    {

        bool deteccionPersona = personaDetectada(trigger_sensor_distancia, echo_sensor_distancia);

        if (deteccionPersona)
        {
            encenderPantalla();
            validadorEspacio();

            if (espacio_disponible)
            {

                if (!compuerta_abierta)
                {
                    abrirCompuerta();
                    compuerta_abierta = true;
                    variable_auxiliar_contenido++;
                    delay(1000);
                    habilitarContenedor();
                }
            }
            else
            {
                if (variable_auxiliar_contenido > 0)
                {
                    procesoEnRiesgo();
                    delay(1000);
                    compuerta_abierta = true;
                }
                else
                {
                    mensajeMantenimiento();
                    conteo_aperturas_fallidos++;
                    Serial.println("\n=============================================");
                    Serial.println(mensaje_contendor_lleno);
                    Serial.println("Se intento recilcar " + String(conteo_aperturas_fallidos) + " veces y no fue posible.");
                    delay(1000);
                    deshabilitarContenedor();
                }
            }
        }
        else
        {

            if (compuerta_abierta)
            {
                encenderPantalla();
                validadorEspacio();

                if (espacio_disponible)
                {
                    reciclajeExitoso();
                    compuerta_abierta = false;
                    conteo_aperturas_exitosos++;
                    Serial.println("\n=============================================");
                    Serial.println("Total aperturas: " + String(conteo_aperturas_exitosos));
                    delay(1000);
                    estadoContenedor();
                }
                else
                {
                    if (variable_auxiliar_contenido > 0)
                    {
                        compuerta_abierta = false;
                        conteo_aperturas_exitosos++;
                        delay(1000);
                        deshabilitarContenedor();
                    }
                    else
                    {
                        if (espacio_disponible)
                        {
                            reciclajeExitoso();
                            compuerta_abierta = false;
                            conteo_aperturas_exitosos++;
                            Serial.println("\n=============================================");
                            Serial.println("Total aperturas: " + String(conteo_aperturas_exitosos));
                            delay(1000);
                            estadoContenedor();
                        }
                        else
                        {
                            contenedorAlMaximo();
                            compuerta_abierta = false;
                            Serial.println("\n=============================================");
                            Serial.println(mensaje_contendor_lleno);
                            delay(1000);
                            deshabilitarContenedor();
                        }
                    }
                }

                cerrarCompuerta();
            }
            variable_auxiliar_contenido = 0; // reset del auxiliar ante cambios en el contenido post reciclaje.
        }
    }
    apagarPantalla();
    delay(900);
}