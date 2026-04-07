# Απλή Γραμμική Παλινδρόμηση: y = (current_temp * 1.05) + 0.5
# Στόχος: Πρόβλεψη επόμενης τιμής θερμοκρασίας
weight = 1.0523
bias = 0.4821

print(f"/* TinyML Model Parameters */")
print(f"#define ML_WEIGHT {weight}f")
print(f"#define ML_BIAS {bias}f")