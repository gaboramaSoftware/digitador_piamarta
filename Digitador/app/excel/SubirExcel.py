import pandas as pd
import hashlib
import sys

def digito_verificador(rut):
    #Calcula el dígito verificador para un RUT numérico (sin puntos ni guión).
    rut_str = str(rut).strip()
    if not rut_str.isdigit():
        raise ValueError("El RUT debe ser numérico")
    reversed_digits = list(map(int, rut_str[::-1]))
    multipliers = [2, 3, 4, 5, 6, 7]
    total = sum(multipliers[i % 6] * digit for i, digit in enumerate(reversed_digits))
    dv = 11 - (total % 11)
    if dv == 11:
        return '0'
    elif dv == 10:
        return 'K'
    else:
        return str(dv)

def procesar_excel_a_sql(ruta_excel, archivo_salida, rut_inicio=20000001):
    """
    Lee un archivo Excel con hojas de cursos y genera script SQL.
    Ignora el Nº de lista y autogenera RUTs válidos a partir del rut_inicio.
    """
    xls = pd.ExcelFile(ruta_excel)
    inserts_usuarios = []
    inserts_details = []
    rut_actual = rut_inicio
    contador_global = 1   

    for hoja in xls.sheet_names:
        print(f"Procesando hoja: {hoja}")
        df = pd.read_excel(xls, sheet_name=hoja, header=None, skiprows=3)

        try:
            # Tomamos las 5 primeras columnas
            df_filtrado = df.iloc[:, [0, 1, 2, 3, 4]]
            df_filtrado.columns = ['rut', 'nombres', 'apaterno', 'amaterno', 'curso']
        except IndexError:
            print("Estructura de columnas inválida. Se requiere: RUN, NOMBRES, A.PATERNO, A.MATERNO, CURSO")
            continue

        df_filtrado = df_filtrado.dropna(subset=['rut', 'nombres'])

        for _, row in df_filtrado.iterrows():
            
            rut_completo = str(row['rut']).strip().replace('.', '')
            nombres = str(row['nombres']).strip()
            if not rut_completo or rut_completo.lower() == 'nan' or not nombres or nombres.lower() == 'nan':
                continue

            if '-' in rut_completo:
                run_id, dv = rut_completo.split('-')
            else:
                run_id = rut_completo[:-1]
                dv = rut_completo[-1] if len(rut_completo) > 0 else "0"

            run_id = ''.join(filter(str.isdigit, run_id))
            dv = dv.upper()

            apaterno = str(row['apaterno']).strip() if pd.notna(row['apaterno']) else ""
            amaterno = str(row['amaterno']).strip() if pd.notna(row['amaterno']) else ""
            curso = str(row['curso']).strip()

            nombre_completo = f"{nombres} {apaterno} {amaterno}".strip().replace('  ', ' ')

            # Usamos ON CONFLICT para NO sobreescribir la huella dactilar si el alumno ya existe
            insert_usuario = f"INSERT INTO Usuarios (run_id, dv, nombre_completo, id_rol, template_huella, activo) VALUES ('{run_id}', '{dv}', '{nombre_completo}', 3, X'', 1) ON CONFLICT(run_id) DO UPDATE SET nombre_completo=excluded.nombre_completo, dv=excluded.dv;"
            inserts_usuarios.append(f"-- #{contador_global}\n{insert_usuario}")

            # Procesar curso y letra
            curso_limpio = "1° Básico" # Default o detectar del nombre de la hoja si es posible
            letra_car = "A"
            
            # Intentar detectar curso y letra del string de la columna 'curso'
            curso_raw = str(row['curso']).strip().lower()
            if curso_raw and curso_raw != 'nan':
                 # Lógica para normalizar el curso al formato de la DB (N° Básico/Medio)
                 if 'primero' in curso_raw or '1' in curso_raw: num = "1°"
                 elif 'segundo' in curso_raw or '2' in curso_raw: num = "2°"
                 elif 'tercer' in curso_raw or '3' in curso_raw: num = "3°"
                 elif 'cuart' in curso_raw or '4' in curso_raw: num = "4°"
                 elif 'quint' in curso_raw or '5' in curso_raw: num = "5°"
                 elif 'sext' in curso_raw or '6' in curso_raw: num = "6°"
                 elif 'sept' in curso_raw or '7' in curso_raw: num = "7°"
                 elif 'octav' in curso_raw or '8' in curso_raw: num = "8°"
                 else: num = "1°"

                 tipo = "Medio" if "medio" in curso_raw or "m" in curso_raw.split() else "Básico"
                 curso_limpio = f"{num} {tipo}"

                 # La letra suele ser la última palabra o carácter
                 partes = curso_raw.split()
                 if len(partes) > 0:
                     letra_raw = partes[-1].upper()
                     if len(letra_raw) == 1 and letra_raw.isalpha():
                         letra_car = letra_raw
            
            # Usar subqueries para obtener los IDs correctos basados en el nombre
            # Si no encuentra coincidencia exacta, el valor será NULL (pero el sistema Go lo manejará)
            insert_detail = f"INSERT OR REPLACE INTO DetailsEstudiante (run_id, id_curso, id_letra) VALUES ('{run_id}', (SELECT id_curso FROM Curso WHERE nombre = '{curso_limpio}' LIMIT 1), (SELECT id_letra FROM Letra WHERE caracter = '{letra_car}' LIMIT 1));"
            inserts_details.append(f"-- #{contador_global}\n{insert_detail}")

            contador_global += 1  

    # Escribir archivo SQL
    with open(archivo_salida, 'w', encoding='utf-8') as f:
        f.write("-- Script generado automáticamente a partir de Excel\n")
        f.write("-- Cada INSERT va precedido de un número de índice (#)\n\n")
        f.write("-- Inserción en tabla Usuarios (estudiantes)\n")
        for ins in inserts_usuarios:
            f.write(ins + "\n")
        f.write("\n-- Inserción en tabla DetailsEstudiante\n")
        for ins in inserts_details:
            f.write(ins + "\n")

    print(f"Archivo SQL generado: {archivo_salida}")
    print(f"Total de estudiantes procesados: {len(inserts_usuarios)}")

    return inserts_usuarios, inserts_details

if __name__ == "__main__":
    if len(sys.argv) > 1:
        ruta_excel = sys.argv[1]
        archivo_salida = sys.argv[2] if len(sys.argv) > 2 else "alumnos.sql"
        procesar_excel_a_sql(ruta_excel, archivo_salida)
    else:
        procesar_excel_a_sql("ASISTENCIA JUNAEB  2025.xlsx", "alumnos.sql")