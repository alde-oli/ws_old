#!/usr/bin/python
# -*- coding: utf-8 -*-
import os

def mylistfiles(directory):
    try:
        files = os.listdir(directory)
        html = "<ul>"
        for file in files:
            html += "<li>{}</li>".format(file)
        html += "</ul>"
        return html
    except Exception as e:
        return "Erreur lors de la lecture du dossier : {}".format(e)

if __name__ == "__main__":
    # Récupérer le chemin du dossier depuis les variables d'environnement
    directory = os.environ.get("directory", "")

    html_files = mylistfiles(directory)

    # Générer la sortie CGI
    print "Content-type: text/html; charset=utf-8\n"
    print "<html><head><title>Liste de Fichiers</title></head><body>"
    print "<h1>Liste de Fichiers dans le Dossier</h1>"
    print html_files
    print "</body></html>"
