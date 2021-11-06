# Installation von einem Grafana-Server mit Influxdb (und MQTT-Schnittstelle)

## Grafana-Installation

Installation nach offizieller Anleitung:
"https://grafana.com/docs/grafana/latest/installation/debian/"
also:

1. PubKey installieren
    sudo apt-get install -y apt-transport-https
    sudo apt-get install -y software-properties-common wget
    wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add - 

2. Repo zu apt hinzufügen
    echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee -a /etc/apt/sources.list.d/grafana.list

3. Paket installieren
    sudo apt-get update
    sudo apt-get install grafana

## Influxdb

Installieren nach offizieller Anleitung:
"https://portal.influxdata.com/downloads/"

### Konfiguration
1. Aufsetzen:
    influx setup
Username und Passwort aussuchen, Namen für die Organisation festlegen. 
Retention am besten auf 0 (infinite) setzen wenn nichts anderes gewünscht

2. Nutzer für Grafana und für die MQTT-Influx-Brücke anlegen, geht am einfachsten über influxdb v1:
    influx v1 auth create --read-bucket {bucketId} --write-bucket {bucketId} --username {username}
Als Usernamen einen anderen als bei 1. nehmen, ein neues passwort muss auch festgelegt werden. 
Die "bucketId" findet man mit folgendem Befehl unter "default"
    influx bucket list

3. InfluxDB-Mapping für influx v1 erstellen
    influx v1 dbrp create --bucket-id {bucketId} --db default --rp default --default

4. Die InfluxDB bei Grafana hinzufuegen. Query Language auf "InfluxQL". Optionen unter Auth alle abstellen. 
Bei "InfluxDB Details" unter Database "default" einstellen. 
Bei User und Password die Daten des in 2. erzeugten Nutzers eingeben und die HTTP Method auf "GET" einstellen.

## Mosquitto

Mosquitto ist der MQTT-Server den wir verwenden. Der Server kann einfach aus dem offiziellen Repo installiert werden, mit
    sudo apt install mosquitto mosquitto-clients

1. Anmeldung einstellen: In der Config-Datei "/etc/mosquitto/mosquitto.conf" folgende Zeilen am ende hinzufügen:
    allow_anonymous false
    password_file /etc/mosquitto/passwordfile

2. Die Passwort-Datei erzeugen: 
    touch /etc/mosquitto/passwordfile

3. Nutzer anlegen. Zuerst muss ein Nutzer mit dem Namen "mqtt" angelegt werden, dieser wird fuer die MQTT-Influx-Brücke benötigt
    mosquitto_passwd /etc/mosquitto/passwordfile {username}
Am Besten legt man auch für jeden sensor einen eigenen Benutzer fest, dadurch kann man die Rechte für jeden einzelnen sensor beschränken.
Außerdem lassen sich die Messwerte so besser zuordnen.

## MQTT-Influxdb Brücke

Das python-Script unter "./mqttInflux.py" stellt die Brücke zwischen Mosquitto und der InfluxDB dar. 

 - Vor dem ersten Starten muss noch folgendes mit pip3 installiert werden
    pip3 install tendo paho-mqtt influxdb

 - Dieses kann einfach in den crontab eingetragen werden:
    crontab -e
Und dann folgendes hinzufuegen:
    */1 * * * * python3 {path to mqttInflux.py}
Dies startet das skript jede Minute neu. Das skript prüft ob bereits eine andere Instanz des skriptes läuft und beendet sich in dem Fall gleich wieder.
Dadurch fällt das skript nicht allzu lange aus falls es mal aufgrund von Verbindungsproblemen oder anderen Schwierigkeiten abstürzt.

 - In dem Skript müssen das Passwort für den MQTT-Zugang (aus Mosquitto.3) sowie der Zugang zur InfluxDB anschließend noch eingetragen werden.

## Anbindung an ESP & Co.

Um Sensoren & Co anzubinden kann jede beliebige mqtt-Bibliothek verwendet werden die das publishen von Werten erlaubt.
Folgdendes Muster sollte für die Topic aber eingehalten werden, damit die Werte im grafana zuzuordnen sind:
    /user/{user}/grafana/{device}/{measurement}
An diese Topic kann entweder direkt eine Zahl geschickt werden (welche dann bei "value" auffindbar ist), oder ein JSON, dessen einzelne Felder dann in grafana abrufbar sind.
