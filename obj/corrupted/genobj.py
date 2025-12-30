import random
for i in range(3000):
    print("v",*(random.random() for i in range(3)))
    print("vt",*(random.random() for i in range(3)))
    print("vn",*(random.random() for i in range(3)))

for i in range(5000):
    print("f", *("/".join(str(random.randrange(1,3000)) for i in range(3)) for j in range(3)))
