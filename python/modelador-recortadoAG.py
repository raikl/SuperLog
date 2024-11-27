import lectura as lt
import modeladorAgenda as modAg
import reporter as rep

# Se importa el modulo de fechas y horas
from datetime import time #para trabajar con horas
from datetime import datetime, date, time, timedelta
import calendar
from dateutil.relativedelta import relativedelta

import os


class Material:
    def __init__(self, idMaterial, descripcion, sector, duracion, pesoCaja, estado, envgranel):
        self.idMaterial = idMaterial
        self.descripcion = descripcion
        self.sector = sector
        self.duracion = duracion
        self.sinPesoCaja = False
        self.sinCajasPallet = True
        self.estaEnDemanda = False
        self.tienePrecio = False
        self.cajasPallet = 0
        self.tiendas = set()
        self.envgranel = envgranel
        if pesoCaja == '(en blanco)':
            self.pesoCaja = 100.0
            self.sinPesoCaja = True
        else:
            self.pesoCaja = pesoCaja
        self.estado = estado
    def __str__(self):
        rtn = "ID: " + str(self.idMaterial) + "; "
        rtn += "DESC: " + self.descripcion + "; "
        rtn += "SECT: " + self.sector + "; "
        rtn += "DUR: " + str(self.duracion) + "; "
        rtn += "PESO: " + str(self.pesoCaja) + "; "
        rtn += "CAJxPAL: " + str(self.cajasPallet) + "; "
        rtn += "EST: " + self.estado
        return rtn
    def estaEsnTienda(self):
        rtn = False
        for e in self.tiendas:
            if isinstance(e, str) and e.startswith('T'):
                rtn = True
                break
        return rtn


def __formatearNodo(codigo):
    nodo = codigo
    if isinstance(nodo, float):
        nodo = int(nodo)
    nodo = str(nodo)
    return nodo

def __generarVarNodo(tipo, codigo, tipoNodoVar):
    nodo = __formatearNodo(codigo)
    if tipo == "Hub":
        rst = "Pl" + nodo
    else:
        rst = tipo[0:2] + nodo        
            
    if tipo not in tipoNodoVar.keys():
        tipoNodoVar[tipo] = {}
    tipoNodoVar[tipo][nodo] = rst
    return rst

def __obtenerVarNodo(nodo, tipos, tipoNodoVar):
    for e in tipos:
        if nodo in tipoNodoVar[e].keys():
            return tipoNodoVar[e][nodo]



def funcProgreso(val):
    pass
    
catalogo = {}
srcRep = {}
nodos = {}
sap = {"SAP1":{}, "SAP2":{}, "SAP3":{}}
sectores = {}
materiales = {}
estadosMaterial = {}
nodoTransportista = {}
tipoNodoVar = {}

catalogo["Nodos"] = lt.leerXLS("datos/Nodos.xlsx", "Nodos", 1, [0, 1, 4, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 3, 2], funcProgreso)
catalogo["Tarifas"] = lt.leerXLS("datos/Tarifas.xlsx", "Tiempos de Viaje", 1, [0, 1, 2], funcProgreso)
catalogo["Rutas"] = lt.leerXLS("datos/rutas.xlsx", "Rutas", 0, [0, 4, 6, 7, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29], funcProgreso)

srcRep["venta-traslado"] = lt.leerCSV("datos/venta-traslado.txt")


carpetaSalida = "E:\\Proyectos\\Agrosuper\\Agendamiento de Transportes\\src\\python\\salida"
dia=30
mes=11
anio=2021
fecha = datetime(anio,mes,dia)
hoy = fecha + relativedelta(days=-1)


for i in range(0, len(catalogo["Nodos"])):
    var = __generarVarNodo(catalogo["Nodos"][i][0], catalogo["Nodos"][i][1], tipoNodoVar)
    abreviacion = ""
    for p in str(catalogo["Nodos"][i][2]).split():
        abreviacion += p[0]
    nodos[var]=(catalogo["Nodos"][i][0], catalogo["Nodos"][i][1], str(catalogo["Nodos"][i][2]), abreviacion, var, catalogo["Nodos"][i][36], catalogo["Nodos"][i][37])

for e in sap.keys():
    sh = lt.leerXLS("datos/Nodos.xlsx", e, 0, [0, 1, 2], funcProgreso)
    c1 = sh[0][1].lower()
    c2 = sh[0][2].lower()
    sap[e][c1] = {}
    sap[e][c2] = {}
    for row_idx in range(1, len(sh)): 
        if len(sh[row_idx][0]) > 0:
            sap[e][c1][sh[row_idx][0]] = sh[row_idx][1]
            sap[e][c2][sh[row_idx][0]] = sh[row_idx][2]

sh = lt.leerXLS("datos/Nodos.xlsx", "Sectores", 1, [0, 1], funcProgreso)
for row_idx in range(0, len(sh)): 
    if len(sh[row_idx][0]) > 0:
        sectores[sh[row_idx][0]] = sh[row_idx][1]

sh = lt.leerXLS("datos/Maestro Materiales.xlsx", "TD", 1, [0, 1, 3, 18, 19, 20, 21, 17], funcProgreso)
for row_idx in range(0, len(sh)): 
    materiales[int(sh[row_idx][0])] = Material(int(sh[row_idx][0]), sh[row_idx][1], sh[row_idx][2], int(sh[row_idx][3]), sh[row_idx][4], sh[row_idx][5], sh[row_idx][6])
    estadosMaterial[sh[row_idx][5].lower()] = sh[row_idx][7]

sh = lt.leerXLS("datos/Tarifas.xlsx", "Transportistas", 1, [0, 1], funcProgreso) 
for row_idx in range(0, len(sh)): 
    var = __obtenerVarNodo(__formatearNodo(sh[row_idx][0]), ["Hub", "Planta", "Sucursal", "Venta Directa"], tipoNodoVar)
    nodoTransportista[var] = int(sh[row_idx][1])


rr, tt, yy, uu, uy = modAg.agendar(catalogo, hoy)
mat1, mat2, mmat3 = atx.atx("ATXT")

srcRep["transportes-cabecera"] = mat1#lt.leerCSV("datos/transportes-cabecera.txt")
srcRep["transportes-detalle"] = mat2#lt.leerCSV("datos/transportes-detalle.txt")
srcRep["capacidades"] = mat3#lt.leerCSV("datos/capacidades.txt")


rp = rep.Reporter(srcRep, nodos, sap, sectores, materiales, estadosMaterial, nodoTransportista, fecha, carpetaSalida)
rp.grabarExportSAP()
rp.grabarZlogTransportes()


print("TERMINO!!")