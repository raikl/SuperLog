# -*- coding: utf-8 -*-
from dateutil.relativedelta import relativedelta
from datetime import datetime
import traceback
import os
import lecturaAG as lt
import modeladorAgenda as modAg
import reporterAG as rep
import atxtAG as atxt
from gurobipy import Model



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
    def str(self):
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



def formatearNodo(codigo):
    nodo = codigo
    if isinstance(nodo, float):
        nodo = int(nodo)
    nodo = str(nodo)
    return nodo



def generarVarNodo(tipo, codigo, tipoNodoVar):
    nodo = formatearNodo(codigo)
    if tipo == "Hub":
        rst = "Pl" + nodo
    else:
        rst = tipo[0:2] + nodo        
            
    if tipo not in tipoNodoVar.keys():
        tipoNodoVar[tipo] = {}
    tipoNodoVar[tipo][nodo] = rst
    return rst



def obtenerVarNodo(nodo, tipos, tipoNodoVar):
    for e in tipos:
        if nodo in tipoNodoVar[e].keys():
            return tipoNodoVar[e][nodo]



def borrar(folder):
    for the_file in os.listdir(folder):
        file_path = os.path.join(folder, the_file)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
            #elif os.path.isdir(file_path): 
            #    shutil.rmtree(file_path)
        except Exception as e:
            print(e)    



class Modelador():
    def __init__(self, catalogo):
        self.catalogo = catalogo
        self.excepcion = None
        self.nodos = {}
        self.sap = {"SAP1":{}, "SAP2":{}, "SAP3":{}}
        self.sectores = {}
        self.materiales = {}
        self.estadosMaterial = {}
        self.nodoTransportista = {}
        self.tipoNodoVar = {}
        self.modAgenda = None
        self.terminarEjecucion = False
        
        
        for i in range(0, len(catalogo["Nodos:Nodos"])):
            var = generarVarNodo(catalogo["Nodos:Nodos"][i][0], catalogo["Nodos:Nodos"][i][1], self.tipoNodoVar)
            abreviacion = ""
            for p in str(catalogo["Nodos:Nodos"][i][2]).split():
                abreviacion += p[0]
            self.nodos[var]=(catalogo["Nodos:Nodos"][i][0], catalogo["Nodos:Nodos"][i][1], str(catalogo["Nodos:Nodos"][i][2]), abreviacion, var, catalogo["Nodos:Nodos"][i][36], catalogo["Nodos:Nodos"][i][37])

        for e in self.sap.keys():
            sh = catalogo["Nodos:" + e]
            c1 = sh[0][1].lower()
            c2 = sh[0][2].lower()
            self.sap[e][c1] = {}
            self.sap[e][c2] = {}
            for row_idx in range(1, len(sh)): 
                if len(sh[row_idx][0]) > 0:
                    self.sap[e][c1][sh[row_idx][0]] = sh[row_idx][1]
                    self.sap[e][c2][sh[row_idx][0]] = sh[row_idx][2]

        sh = catalogo["Nodos:Sectores"]
        for row_idx in range(0, len(sh)): 
            if len(sh[row_idx][0]) > 0:
                self.sectores[sh[row_idx][0]] = sh[row_idx][1]

        sh = catalogo["MaestroMateriales:TD"]
        for row_idx in range(0, len(sh)): 
            self.materiales[int(sh[row_idx][0])] = Material(int(sh[row_idx][0]), sh[row_idx][1], sh[row_idx][2], int(sh[row_idx][3]), sh[row_idx][4], sh[row_idx][5], sh[row_idx][6])
            self.estadosMaterial[sh[row_idx][5].lower()] = sh[row_idx][7]

        sh = catalogo["Tarifas:Transportistas"]
        for row_idx in range(0, len(sh)): 
            var = obtenerVarNodo(formatearNodo(sh[row_idx][0]), ["Hub", "Planta", "Sucursal", "Venta Directa"], self.tipoNodoVar)
            self.nodoTransportista[var] = int(sh[row_idx][1])

        self.crearCarpetaSalida()
        ########################################################
        self.printCatalogo()
        ########################################################


    def crearCarpetaSalida(self):
        self.carpetaSalida = self.catalogo["Carpeta de Salida"]
        self.pathTmp = self.carpetaSalida + "/tmp"
        self.pathLog = self.carpetaSalida + "/log"
        self.pathSap = self.carpetaSalida + "/SAP"
        borrar(self.carpetaSalida)
        lp = [self.pathTmp, self.pathLog, self.pathSap]
        for p in lp:
            if os.path.isdir(p):
                borrar(p)
                #print()
            else:    
                os.mkdir(p)


    def printCatalogo(self):
        for l in self.catalogo:
            print(l, ":")
            if isinstance(self.catalogo[l], list):
                print(self.catalogo[l][0])
                print(self.catalogo[l][1])
            else:
                print(self.catalogo[l])


    def procesarP1Preparativos(self):
        self.modAgenda = modAg.Agenda(self.pathLog)
        
        #try:
        self.fecha = self.catalogo["Fecha"] #datetime.strptime(self.catalogo["Fecha"], '%d-%m-%Y').date()
        hoy = self.fecha + relativedelta(days=-1)
        self.terminarEjecucion = self.modAgenda.preparativos(self.catalogo, hoy)
        #except Exception as e:
        #    print(traceback.format_exc())
        #    self.excepcion = str(e)


    def procesarP1Agendar(self):
        #try:
        if self.catalogo["Env"] == None:
            self.tm = Model('ppl')
        else:
            self.tm = Model('ppl', env=self.catalogo["Env"])
        self.tm.setParam('LogFile', self.pathLog + '/Lognobaja.log')
        self.tm.setParam('TimeLimit', self.catalogo["Tiempo Límite"])
        self.itinerario = self.modAgenda.agendar(self.tm)
        #except Exception as e:
        #    print(traceback.format_exc())
        #    self.excepcion = str(e)


    def procesarP2(self):
        #try:
        mat1, mat2 = atxt.generar(self.catalogo, self.itinerario)
        self.srcRep = {}
        self.srcRep["transportes-cabecera"] = self.catalogo["transportes-cabecera"]
        self.srcRep["transportes-detalle"] = mat1
        self.srcRep["capacidades"] = mat2
        self.srcRep["venta-traslado"] = self.catalogo["venta-traslado"]
        #except Exception as e:
        #    print(traceback.format_exc())
        #    self.excepcion = str(e)

    def procesarP3(self):
        #try:        
        rp = rep.Reporter(
            self.srcRep, 
            self.nodos, 
            self.sap, 
            self.sectores, 
            self.materiales, 
            self.estadosMaterial, 
            self.nodoTransportista, 
            self.fecha, 
            self.catalogo["Carpeta de Salida"]
        )
        rp.grabarExportSAP()
        rp.grabarZlogTransportes()
        rp.grabarIndicadores()
        #except Exception as e:
        #    print(traceback.format_exc())
        #    self.excepcion = str(e)
