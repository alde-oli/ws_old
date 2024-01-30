#!/usr/bin/python
# -*- coding: utf-8 -*-
import os

def addition(param1, param2):
    try:
        result = int(param1) + int(param2)
        return result
    except ValueError:
        return "Invalid"

if __name__ == "__main__":
    # Récupérer les valeurs depuis les variables d'environnement
    param1 = os.environ.get("num1", "")
    param2 = os.environ.get("num2", "")

    result = addition(param1, param2)

    data = """
    html code
    """
    print(data)
    # Générer la sortie CGI
    print("Content-type: text/html; charset=utf-8\n")
    print("<html><head><title>CGI Python</title></head><body>")
    print("<h1>Addition</h1>")
    print("<p>Result is {}</p>".format(result))
    print("</body></html>")
