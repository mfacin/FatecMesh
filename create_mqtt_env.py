from urllib.error import HTTPError, URLError
import urllib.request
import json

BASE_URL = "fatecmesh.tech"


def check_broker_health():
    try:
        res = urllib.request.urlopen(f"http://{BASE_URL}:4041/iot/about")
        print(res.read())
        res.close()
        return True

    except URLError as url_e:
        print(f"Could not check broker health due an URL error: {str(url_e)}")
        return False

    except HTTPError as http_e:
        print(f"Could not check broker health due a HTTP error: {str(url_e)}")
        return False

    except Exception as e:
        print(f"Could not check broker health due an unknown error: {str(e)}")
        return False


def check_if_service_group():
    req = urllib.request.Request(f"http://{BASE_URL}:4041/iot/services")
    req.add_header("fiware-service", "fatecmesh")
    req.add_header("fiware-servicepath", "/")

    try:
        res = urllib.request.urlopen(req)
        body = json.loads(res.read())
        res.close()
        return body['count']

    except URLError as url_e:
        print(f"Could not check broker health due an URL error: {str(url_e.reason)}")
        return -1

    except HTTPError as http_e:
        print(f"Could not check broker health due a HTTP error: {str(url_e.reason)}")
        return -1

    except Exception as e:
        print(f"Could not check broker health due an unknown error: {str(e.reason)}")
        return -1


def create_service_group():
    body = json.dumps({
        "services": [{
            "apikey":      "fatecmeshiot",
            "cbroker":     "http://orion:1026",
            "entity_type": "Station",
            "resource":    ""
        }]   
    }).encode("utf-8")

    req = urllib.request.Request(f"http://{BASE_URL}:4041/iot/services/")
    req.add_header("Content-Type", "application/json; charset=utf-8")
    req.add_header("Content-Length", len(body))
    req.add_header("fiware-service", "fatecmesh")
    req.add_header("fiware-servicepath", "/")

    try:
        res = urllib.request.urlopen(req, body)
        print(res.read())
        res.close()
        return True

    except URLError as url_e:
        print(f"Could not create service group due an URL error: {str(url_e.reason)}")
        return False

    except HTTPError as http_e:
        print(f"Could not create service group due a HTTP error: {str(url_e.reason)}")
        return False

    except Exception as e:
        print(f"Could not create service group due an unknown error: {str(e.reason)}")
        return False


def check_if_devices():
    req = urllib.request.Request(f"http://{BASE_URL}:4041/iot/devices?options=keyValues")
    req.add_header("fiware-service", "fatecmesh")
    req.add_header("fiware-servicepath", "/")

    try:
        res = urllib.request.urlopen(req)
        devices = json.loads(res.read())
        res.close()
        return devices["count"]

    except URLError as url_e:
        print(f"Could not check broker health due an URL error: {str(url_e.reason)}")
        return -1

    except HTTPError as http_e:
        print(f"Could not check broker health due a HTTP error: {str(url_e.reason)}")
        return -1

    except Exception as e:
        print(f"Could not check broker health due an unknown error: {str(e.reason)}")
        return -1
    

def create_devices():
    ids = ["5BCB18", "5B4A88", "49B7AC", "5AF2F8", "E0C3E4"]
    data = {
        "devices": []
    }

    for dev_id in ids:
        data["devices"].append({
            "device_id":   f"Station{dev_id}",
            "entity_name": f"urn:ngsi-ld:Station:{dev_id}",
            "entity_type": "Station",
            "protocol":    "PDI-IoTA-UltraLight",
            "transport":   "MQTT",
            "timezone":    "America/Sao_Paulo",
            "attributes": [
                { "object_id": "t", "name": "temperature", "type": "Integer" },
                { "object_id": "p", "name": "pressure", "type": "Integer" },
                { "object_id": "ah", "name": "humidity", "type": "Integer" },
                { "object_id": "pc", "name": "precipitation", "type": "Integer" },
                { "object_id": "w", "name": "windSpeed", "type": "Integer" },
                { "object_id": "l", "name": "luminosity", "type": "Integer" },
                { "object_id": "gh", "name": "groundHumidity", "type": "Integer" }
            ],
            "commands": [
                { "name": "switch", "type": "command" }
            ]
        })

    body = json.dumps(data).encode("utf-8")

    req = urllib.request.Request(f"http://{BASE_URL}:4041/iot/devices")
    req.add_header("Content-Type", "application/json; charset=utf-8")
    req.add_header("Content-Length", len(body))
    req.add_header("fiware-service", "fatecmesh")
    req.add_header("fiware-servicepath", "/")

    try:
        res = urllib.request.urlopen(req, body)
        print(res.read())
        res.close()
        return True

    except URLError as url_e:
        print(f"Could not create devices due an URL error: {str(url_e.reason)}")
        return False

    except HTTPError as http_e:
        print(f"Could not create devices due a HTTP error: {str(url_e.reason)}")
        return False

    except Exception as e:
        print(f"Could not create devices due an unknown error: {str(e.reason)}")
        return False


if __name__ == "__main__":
    # checking broker health
    print("> Checking Broker Health")
    if not check_broker_health():
        exit()
    

    # service group
    print("\n> Creating Service Group")
    service_count = check_if_service_group()

    if service_count < 0:
        exit()
    elif service_count > 0:
        print("> Service Group already created")
    elif service_count == 0:
        if not create_service_group():
            exit()

    
    # devices
    print("\n> Creating Devices")
    service_count = check_if_devices()

    if service_count < 0:
        exit()
    elif service_count > 0:
        print("> Devices already created")
    elif service_count == 0:
        if not create_devices():
            exit()
