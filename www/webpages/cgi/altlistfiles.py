#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import cgi

def list_files(directory):
    try:
        files = os.listdir(directory)
        html = "<ul>"
        for file in files:
            html += "<li>{}</li>".format(file)
        html += "</ul>"
        return html
    except OSError as e:
        return "Erreur lors de la lecture du dossier: {}".format(e)

if __name__ == "__main__":

    # Utiliser cgi.FieldStorage pour récupérer les paramètres GET
    form = cgi.FieldStorage()
    directory = os.environ.get("directory", "")

    html_files = list_files(directory)

    print "<html><head><title>Liste de Fichiers</title></head><body>"
    print "<h1>Liste de Fichiers dans le Dossier</h1>"
    print html_files
    print "</body></html>"
