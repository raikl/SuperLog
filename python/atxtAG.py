# -*- coding: utf-8 -*-
from gurobipy import quicksum



def generar(catalogo, itinerario):
    mat1 = []
    mat2 = []

    for e in catalogo["transportes-detalle"]:
        e[2] = itinerario[e[0], e[1]][0]
        e[3] = itinerario[e[0], e[1]][1]
        mat1.append(e)
    for e in catalogo["capacidades"]:
        e[5] = itinerario[e[0], e[3]][0]
        e[6] = itinerario[e[0], e[3]][1]
        mat2.append(e)

    return mat1, mat2