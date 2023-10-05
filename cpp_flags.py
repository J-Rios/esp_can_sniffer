
Import("env")

env.Append(
    CXXFLAGS=[
        "-Weffc++"
#        "-pedantic",
    ]
)

#print(env.get("CXXFLAGS"))
