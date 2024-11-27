import sys
import lecturaAG as lectura
import modeladorAG as mod


class InterfazSilenciosa():
    def run(self, inputsTxt, maestroMaterialesXls, nodosXls, tarifasXls, carpetaSalida, tiempo, fecha, env):
        self.catalogo = {}
        self.catalogo["venta-traslado"] = lectura.leerCSV(inputsTxt + "/venta-traslado.txt", self.actualizarProgreso)
        self.catalogo["capacidades"] = lectura.leerCSV(inputsTxt + "/capacidades.txt", self.actualizarProgreso)
        self.catalogo["transportes-cabecera"] = lectura.leerCSV(inputsTxt + "/transportes-cabecera.txt", self.actualizarProgreso)
        self.catalogo["transportes-detalle"] = lectura.leerCSV(inputsTxt + "/transportes-detalle.txt", self.actualizarProgreso)
        self.catalogo["Rutas"] = lectura.leerCSV2(inputsTxt + "/Rutas.txt", self.actualizarProgreso)
    
        hojas = [
            ["TD", 1, [0, 1, 3, 18, 19, 20, 21, 17]]
        ]
        self.catalogo.update(lectura.leerXLS(maestroMaterialesXls, "MaestroMateriales", hojas, self.actualizarProgreso))

        hojas = [
            ["Nodos", 1, [0, 1, 4, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 3, 2]], 
            ["SAP1", 0, [0, 1, 2]],
            ["SAP2", 0, [0, 1, 2]],
            ["SAP3", 0, [0, 1, 2]],
            ["Sectores", 1, [0, 1]]
        ]
        self.catalogo.update(lectura.leerXLS(nodosXls, "Nodos", hojas, self.actualizarProgreso))

        hojas = [
            ["Tiempos de Viaje", 1, [0, 1, 2]],
            ["Transportistas", 1, [0, 1]]
        ]
        self.catalogo.update(lectura.leerXLS(tarifasXls, "Tarifas", hojas, self.actualizarProgreso))
        
        self.catalogo["Carpeta de Salida"] = carpetaSalida
        self.catalogo["Fecha"] = fecha
        self.catalogo["Tiempo Límite"] = tiempo
        self.catalogo["Env"] = env
        
        pasos = mod.Modelador(self.catalogo)
        pasos.procesarP1Preparativos()
        pasos.procesarP1Agendar()
        pasos.procesarP2()
        pasos.procesarP3()
        


    def actualizarProgreso(self, avance):
        pass



#if __name__ == '__main__':
#    interfazSilenciosa = InterfazSilenciosa()
#    interfazSilenciosa.run(
#        "C:/Users/thi-s/OneDrive/Desktop/salidaUnificado/logistica/tmp", 
#        "C:/Users/thi-s/OneDrive/Desktop/AgroPack-main/Unificado/src/python/Transporte/Maestro Materiales.xlsx",
#        "C:/Users/thi-s/OneDrive/Desktop/AgroPack-main/Unificado/src/python/Transporte/Nodos.xlsx",
#        "C:/Users/thi-s/OneDrive/Desktop/AgroPack-main/Unificado/src/python/Transporte/Tarifas.xlsx"
#        )