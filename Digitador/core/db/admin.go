package db

type MockDB struct {
	ObtenerTodosTemplatesFunc func() (map[string][]byte, error)
	GetUserFunc               func(runID string) (*Usuario, error)
}

func (m *MockDB) ObtenerTodosTemplates() (map[string][]byte, error) {
	if m.ObtenerTodosTemplatesFunc != nil {
		return m.ObtenerTodosTemplatesFunc()
	}
	return nil, nil // o un error por defecto
}

func (m *MockDB) GetUser(runID string) (*Usuario, error) {
	if m.GetUserFunc != nil {
		return m.GetUserFunc(runID)
	}
	return nil, nil
}
