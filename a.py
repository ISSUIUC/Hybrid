import numpy as np
import matplotlib.pyplot as plt

t = np.linspace(0,1,1000000)
n = np.sin(t * 2 * np.pi * 261.6256)
p = (n+np.random.random(1000000)*5)
# plt.plot(n)
# plt.plot(n > 0)
plt.plot(n)
plt.plot(p)
# plt.plot(np.fft.fft(n))
# plt.plot(np.fft.fft(p > 0))

plt.show()