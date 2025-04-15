import subprocess

file_path = "filmes.csv"

# Data to add
data = """0dcb3313-ad68-4cab-8629-61a3c95da350,Origem,sci-fi,Christpher Nolan,2010
1ce51ba3-1945-47ae-97c0-7b541e22235e,Orgulho e preconceito,romance,Joe Wright,2005
06e03881-3a14-43f9-937a-5f55f2677294,Blade Runner 2049,sci-fi,Denis Villeneuve,2017
"""

# Clear the file and write new data
with open(file_path, "w") as file:
    file.write(data)

print(f"File '{file_path}' has been cleared and updated.")


# Define the command and arguments
commands = [
    ["./client", "127.0.0.1", "listar_por_genero", "sci-fi"],
    ["./client", "127.0.0.1", "listar_tudo"],
    ["./client", "127.0.0.1", "listar_titulos"],
    ["./client", "127.0.0.1", "listar_detalhes", "0dcb3313-ad68-4cab-8629-61a3c95da350"],
    ["./client", "127.0.0.1", "atualizar_genero", "Orgulho e preconceito", "drama"],
    ["./client", "127.0.0.1", "remover", "0dcb3313-ad68-4cab-8629-61a3c95da350"],
    ["./client", "127.0.0.1", "adicionar_filme", "nome", "genero", "diretor", "ano"],
]

for command in commands:
    try:
        result = subprocess.run(command, check=True, capture_output=True, text=True)
        print(result.stdout)
    except subprocess.CalledProcessError as e:
        print(e.stderr)
