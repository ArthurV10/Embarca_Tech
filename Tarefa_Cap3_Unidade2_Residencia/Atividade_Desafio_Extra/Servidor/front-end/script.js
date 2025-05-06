// Função para converter direção para graus
function direcaoParaGraus(direcao) {
    const mapa = {
        'Norte': 0,
        'Nordeste': 45,
        'Leste': 90,
        'Sudeste': 135,
        'Sul': 180,
        'Sudoeste': 225,
        'Oeste': 270,
        'Noroeste': 315
    };
    return mapa[direcao] ?? null;
}

// Função para atualizar os dados dos sensores
async function atualizarDados() {
    try {
        // Para demonstração, usaremos dados simulados
        // Em produção, descomente o código de fetch e comente o mockData
        
        const response = await fetch('/data');
        const data = await response.json();
        
        // Dados simulados para demonstração
        // const data = getMockData();
        
        // Atualiza valores dos sensores
        document.getElementById('x').textContent = data.x;
        document.getElementById('y').textContent = data.y;
        
        // Atualiza estados dos botões com código de cores
        const btnA = document.getElementById('btn_a');
        btnA.textContent = data.btn_a ? 'PRESSIONADO' : 'LIVRE';
        btnA.className = data.btn_a ? 'valor status-active' : 'valor status-inactive';
        
        const btnB = document.getElementById('btn_b');
        btnB.textContent = data.btn_b ? 'PRESSIONADO' : 'LIVRE';
        btnB.className = data.btn_b ? 'valor status-active' : 'valor status-inactive';
        
        document.getElementById('gas').textContent = data.gas;
        document.getElementById('dir').textContent = data.dir;

        // Atualiza a seta da bússola
        const graus = direcaoParaGraus(data.dir);
        const arrow = document.getElementById('arrow');
        if (graus !== null) {
            arrow.style.display = 'block';
            arrow.style.transform = `rotate(${graus}deg)`;
        } else {
            arrow.style.display = 'none';
        }

    } catch (error) {
        console.error('Erro:', error);
    }
}

// Função para gerar dados simulados para demonstração
function getMockData() {
    const directions = ['Norte', 'Nordeste', 'Leste', 'Sudeste', 'Sul', 'Sudoeste', 'Oeste', 'Noroeste'];
    return {
        x: Math.floor(Math.random() * 100),
        y: Math.floor(Math.random() * 100),
        btn_a: Math.random() > 0.5,
        btn_b: Math.random() > 0.5,
        gas: Math.floor(Math.random() * 1000),
        dir: directions[Math.floor(Math.random() * directions.length)]
    };
}

// Adiciona efeito de pixel às caixas
function addPixelEffect() {
    const boxes = document.querySelectorAll('.sensor-box, .compass-box, .title-container');
    
    boxes.forEach(box => {
        box.addEventListener('mouseover', () => {
            box.style.transition = 'all 0.2s ease';
            box.style.transform = 'translate(-2px, -2px)';
            box.style.boxShadow = '6px 6px 0 rgba(0, 0, 0, 0.8), 10px 10px 0 rgba(0, 0, 0, 0.4)';
        });
        
        box.addEventListener('mouseout', () => {
            box.style.transition = 'all 0.2s ease';
            box.style.transform = '';
            box.style.boxShadow = '4px 4px 0 rgba(0, 0, 0, 0.8), 8px 8px 0 rgba(0, 0, 0, 0.4)';
        });
    });
}

// Inicialização
document.addEventListener('DOMContentLoaded', () => {
    // Adiciona efeito de pixel
    addPixelEffect();
    
    // Atualização inicial de dados
    atualizarDados();
    
    // Define intervalo para atualizações de dados
    setInterval(atualizarDados, 1000);
});