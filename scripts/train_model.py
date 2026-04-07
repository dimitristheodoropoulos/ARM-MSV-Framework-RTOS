# train_model.py — No-dependency Linear Regression
# Υπολογισμός με τη μέθοδο των ελαχίστων τετραγώνων (Least Squares)

x_data = [30.0, 31.0, 32.5, 34.0, 35.5]
y_data = [31.0, 32.5, 34.0, 35.5, 37.0]

n = len(x_data)
sum_x = sum(x_data)
sum_y = sum(y_data)
sum_xy = sum(x * y for x, y in zip(x_data, y_data))
sum_xx = sum(x * x for x in x_data)

# Υπολογισμός Weight (w) και Bias (b)
w = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x**2)
b = (sum_y - w * sum_x) / n

print(f"--- Training Results ---")
print(f"Weight (w): {w:.4f}")
print(f"Bias (b):   {b:.4f}")
print(f"Formula:    y = {w:.4f} * x + {b:.4f}")