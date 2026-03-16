//ELIMINAR EN PRODUCCION, LO SUPLICO

package digitador

import "time"

type Sensor interface {
	CapturarHuella() ([]byte, error)
	CapturarHuellaTimeout(timeout time.Duration) ([]byte, error)
	CompararHuellas(plantilla1 []byte, plantilla2 []byte) (int, error)
	Cerrar()
}

type MockSensor struct{}

func (m *MockSensor) CapturarHuella() ([]byte, error) {
	//simula que siempre detecta una plantilla con el dedo
	return []byte{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}, nil
}

func (m *MockSensor) CapturarHuellaTimeout(timeout time.Duration) ([]byte, error) {
	//simula que siempre detecta una plantilla con el dedo
	return []byte{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10}, nil
}

func (m *MockSensor) CompararHuellas(plantilla1 []byte, plantilla2 []byte) (int, error) {
	//simula que siempre detecta una plantilla con el dedo
	return 100, nil
}

func (m *MockSensor) Cerrar() {
	//simula que siempre detecta una plantilla con el dedo
}
